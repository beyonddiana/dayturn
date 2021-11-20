/**
 * @file daeexport.cpp
 * @brief A system which allows saving in-world objects to Collada DAE format files for offline texturing or conversion to mesh etc.  Ported from Singularity by Jessica Wabbit.
 * @authors Latif Khalifa, Jessica Wabbit
 *
 * $LicenseInfo:firstyear=2013&license=LGPLV2.1$
 * Copyright (C) 2013 Latif Khalifa
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */
#include "llviewerprecompiledheaders.h"

#include "daeexport.h"

//
//	colladadom includes
//
#if LL_MSVC
#pragma warning (disable : 4018)
#pragma warning (push)
#pragma warning (disable : 4068)
#pragma warning (disable : 4263)
#pragma warning (disable : 4264)
#endif

#include "dae.h"
#include "dom/domAsset.h"
#include "dom/domBind_material.h"
#include "dom/domCOLLADA.h"
#include "dom/domConstants.h"
#include "dom/domController.h"
#include "dom/domEffect.h"
#include "dom/domGeometry.h"
#include "dom/domInstance_geometry.h"
#include "dom/domInstance_material.h"
#include "dom/domInstance_node.h"
#include "dom/domInstance_effect.h"
#include "dom/domMaterial.h"
#include "dom/domMatrix.h"
#include "dom/domNode.h"
#include "dom/domProfile_COMMON.h"
#include "dom/domRotate.h"
#include "dom/domScale.h"
#include "dom/domTranslate.h"
#include "dom/domVisual_scene.h"

#if LL_MSVC
#pragma warning (pop)
#endif

#include "boost/date_time/posix_time/posix_time.hpp"

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llappviewer.h"
#include "llcallbacklist.h"
#include "lldatapacker.h"
#include "llfilepicker.h"
#include "llmeshrepository.h"
#include "llsdserialize.h"
#include "llsdutil_math.h"
#include "llsdutil.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewernetwork.h"
#include "llvovolume.h"
#include "llworld.h"
#include "llnotificationsutil.h"

#define ANY_FACE -1

extern LLUUID gAgentID;

namespace DAEExportUtil
{
	bool can_export_node(LLSelectNode *node)
	{
		LLViewerObject *object = node->getObject();

		if (object->permYouOwner() && node->mPermissions->getCreator() == gAgentID) {
			return true;
		}
		return false;
	}

	//
	//	returns true to enable the export menu items if at least
	//	one object in the linkset is exportable.  The exporter can
	//	take it from there and check permissions on the rest of the
	//	linkset
	//
	bool enable_export_object()
	{
        if (LLSelectMgr::getInstance()->selectGetAllValid() && gSavedSettings.getbool("FSEnableObjectExports"))
		{
			LLObjectSelectionHandle selection = LLSelectMgr::instance().getSelection();

			for (LLObjectSelection::iterator iter = selection->begin(); iter != selection->end(); iter++) {
				LLSelectNode *node = *iter;

				if (can_export_node(node) && node->getObject()->getVolume()) 
				{
					return true;
				}
			}
		}

		return false;
	}


	void export_selection()
	{
		LLObjectSelectionHandle selection = LLSelectMgr::instance().getSelection();
		if (!selection) {
			LL_WARNS("daeexport") << "No objects selected." << LL_ENDL;
			return;
		}

		LLViewerObject *root = selection->getFirstRootObject();
		LLObjectSelection::valid_root_iterator root_it = selection->valid_root_begin();
		LLSelectNode *node = *root_it;

		if (!root || !node) {
			root = selection->getFirstObject();
			LLObjectSelection::iterator iter = selection->begin();
			node = *iter;

			if (!root || !node) {
				LL_WARNS("daeexport") << "No objects selected for export." << LL_ENDL;
				return;
			}
		}

		DAESaver *daesaver = new DAESaver;
		daesaver->mOffset = -(root->getRenderPosition());

		std::string filename = node->mName;

		S32 total = 0;
		S32 included = 0;

		for (LLObjectSelection::iterator iter = selection->begin(); iter != selection->end(); iter++) {
			node = *iter;
			total++;

			if (!can_export_node(node) || !node->getObject()->getVolume()) {
				continue;
			}

			included++;
			daesaver->add(node->getObject(), node->mName);
		}

		if (daesaver->mObjects.empty()) {
			LLNotificationsUtil::add("DAEExportFailed");
			return;
		}

		LLFilePicker &file_picker = LLFilePicker::instance();

		if (!file_picker.getSaveFile(LLFilePicker::FFSAVE_DAE, LLDir::getScrubbedFileName(filename + ".dae"))) {
			LL_WARNS("daeexport") << "No file selected." << LL_ENDL;
			return;
		}

		daesaver->save_collada_dae(file_picker.getFirstFile());

		LLSD args;
		args["TOTAL"] = total;
		args["FAILED"] = total - included;

		LLNotificationsUtil::add(
		    included == total ? "DAEExportComplete" : "DAEExportPartial",
		    args
		);
	}
}

void DAESaver::add(const LLViewerObject *prim, const std::string name)
{
	mObjects.push_back(std::pair<LLViewerObject*,std::string>((LLViewerObject*)prim, name));
}

class v4adapt
{
private:
	LLStrider<LLVector4a> mV4aStrider;
public:
	v4adapt(LLVector4a *vp){ mV4aStrider = vp; }
	inline LLVector3 operator[] (const unsigned int i) { return LLVector3((F32*)&mV4aStrider[i]); }
};

void DAESaver::add_source(daeElement *mesh, const char *src_id, std::string params, const std::vector<F32> &vals)
{
	daeElement *source = mesh->add("source");
	source->setAttribute("id", src_id);
	daeElement *src_array = source->add("float_array");

	src_array->setAttribute("id", llformat("%s-%s", src_id, "array").c_str());
	src_array->setAttribute("count", llformat("%d", vals.size()).c_str());

	for (S32 i = 0; i < vals.size(); i++) {
		((domFloat_array*)src_array)->getValue().append(vals[i]);
	}

	domAccessor *acc = daeSafeCast<domAccessor>(source->add("technique_common accessor"));
	acc->setSource(llformat("#%s-%s", src_id, "array").c_str());
	acc->setCount(vals.size() / params.size());
	acc->setStride(params.size());

	for (std::string::iterator p_iter = params.begin(); p_iter != params.end(); ++p_iter) {
		domElement *pX = acc->add("param");
		pX->setAttribute("name", llformat("%c", *p_iter).c_str());
		pX->setAttribute("type", "float");
	}
}

void DAESaver::add_polygons(daeElement *mesh, const char *geomID, const char *materialID, LLViewerObject *obj, int face_to_include)
{
	domPolylist *polylist = daeSafeCast<domPolylist>(mesh->add("polylist"));
	polylist->setMaterial(materialID);

	//
	//	vertices semantic
	//
	{
		domInputLocalOffset *input = daeSafeCast<domInputLocalOffset>(polylist->add("input"));
		input->setSemantic("VERTEX");
		input->setOffset(0);
		input->setSource(llformat("#%s-%s", geomID, "vertices").c_str());
	}

	//
	//	normals semantic
	//
	{
		domInputLocalOffset *input = daeSafeCast<domInputLocalOffset>(polylist->add("input"));
		input->setSemantic("NORMAL");
		input->setOffset(0);
		input->setSource(llformat("#%s-%s", geomID, "normals").c_str());
	}

	//
	//	UV semantic
	//
	{
		domInputLocalOffset *input = daeSafeCast<domInputLocalOffset>(polylist->add("input"));
		input->setSemantic("TEXCOORD");
		input->setOffset(0);
		input->setSource(llformat("#%s-%s", geomID, "map0").c_str());
	}

	//
	//	save indices
	//
	domP *p = daeSafeCast<domP>(polylist->add("p"));
	domPolylist::domVcount *vcount = daeSafeCast<domPolylist::domVcount>(polylist->add("vcount"));
	S32 index_offset = 0;
	S32 num_tris = 0;

	for (S32 face_num = 0; face_num < obj->getVolume()->getNumVolumeFaces(); face_num++) {
		const LLVolumeFace *face = &obj->getVolume()->getVolumeFace(face_num);

		if (face_to_include == ANY_FACE || face_to_include == face_num) {
			for (S32 i = 0; i < face->mNumIndices; i++) {
				U16 index = index_offset + face->mIndices[i];
				(p->getValue()).append(index);
				if (i % 3 == 0) {
					(vcount->getValue()).append(3);
					num_tris++;
				}
			}
		}
		index_offset += face->mNumVertices;
	}
	polylist->setCount(num_tris);
}

bool DAESaver::save_collada_dae(std::string filename)
{
	DAE dae;

	//
	//	first set the filename to save
	//
	daeElement *root = dae.add(filename);

	//
	//	obligatory elements in header
	//
	daeElement *asset = root->add("asset");

	//
	//	get ISO format time
	//
	// Get ISO format time
	time_t rawtime;
	time(&rawtime);
	struct tm* utc_time = gmtime(&rawtime);
	std::string date = llformat("%04d-%02d-%02dT%02d:%02d:%02d", utc_time->tm_year + 1900, utc_time->tm_mon + 1, utc_time->tm_mday, utc_time->tm_hour, utc_time->tm_min, utc_time->tm_sec);
	daeElement* created = asset->add("created");
	created->setCharData(date);
	daeElement* modified = asset->add("modified");
	modified->setCharData(date);
	daeElement* unit = asset->add("unit");
	unit->setAttribute("name", "meter");
	unit->setAttribute("value", "1");
	daeElement* up_axis = asset->add("up_axis");
	up_axis->setCharData("Z_UP");
	//
	//	file creator
	//
	LLAvatarName avatar_name;
	std::string author_name;

	if (LLAvatarNameCache::get(gAgentID, &avatar_name)) {
		author_name = avatar_name.getLegacyName() + " (" + LLGridManager::getInstance()->getGridLabel() + ")";
	}
	else {
		author_name = LLAppViewer::instance()->getSecondLifeTitle() + " User";
	}

	daeElement *contributor = asset->add("contributor");
	contributor->add("author")->setCharData(author_name);
	contributor->add("authoring_tool")->setCharData(LLAppViewer::instance()->getSecondLifeTitle() + " Collada DAE Export");

	daeElement *geomLib = root->add("library_geometries");
	daeElement *effects = root->add("library_effects");
	daeElement *materials = root->add("library_materials");
	daeElement *scene = root->add("library_visual_scenes visual_scene");
	scene->setAttribute("id", "Scene");
	scene->setAttribute("name", "Scene");

	S32 prim_no = 0;

	for (obj_info_t::iterator obj_iter = mObjects.begin(); obj_iter != mObjects.end(); obj_iter++) {
		LLViewerObject *obj = obj_iter->first;
		S32 total_num_vertices = 0;

		std::string name = llformat("prim%d", prim_no++);
		const char *geomID = name.c_str();

		daeElement *geom = geomLib->add("geometry");
		geom->setAttribute("id", llformat("%s-%s", geomID, "mesh").c_str());
		daeElement *mesh = geom->add("mesh");

		std::vector<F32> position_data;
		std::vector<F32> normal_data;
		std::vector<F32> uv_data;

		S32 num_faces = obj->getVolume()->getNumVolumeFaces();

		for (S32 face_num = 0; face_num < num_faces; face_num++) {
			const LLVolumeFace *face = &obj->getVolume()->getVolumeFace(face_num);
			total_num_vertices += face->mNumVertices;

			v4adapt verts(face->mPositions);
			v4adapt norms(face->mNormals);
			for (S32 i=0; i < face->mNumVertices; i++) {
				const LLVector3 v = verts[i];
				position_data.push_back(v.mV[VX]);
				position_data.push_back(v.mV[VY]);
				position_data.push_back(v.mV[VZ]);

				const LLVector3 n = norms[i];
				normal_data.push_back(n.mV[VX]);
				normal_data.push_back(n.mV[VY]);
				normal_data.push_back(n.mV[VZ]);

				const LLVector2 uv = face->mTexCoords[i];
				uv_data.push_back(uv.mV[VX]);
				uv_data.push_back(uv.mV[VY]);
			}
		}


		add_source(mesh, llformat("%s-%s", geomID, "positions").c_str(), "XYZ", position_data);
		add_source(mesh, llformat("%s-%s", geomID, "normals").c_str(), "XYZ", normal_data);
		add_source(mesh, llformat("%s-%s", geomID, "map0").c_str(), "ST", uv_data);

		//
		//	add the <vertices> element
		//
		daeElement *vn = mesh->add("vertices");
		vn->setAttribute("id", llformat("%s-%s", geomID, "vertices").c_str());
		daeElement *vi = vn->add("input");
		vi->setAttribute("semantic", "POSITION");
		vi->setAttribute("source", llformat("#%s-%s", geomID, "positions").c_str());

		for (S32 face_num = 0; face_num < num_faces; face_num++) {
			//
			//	add triangles
			//
			add_polygons(mesh, geomID, llformat("%s-f%d-%s", geomID, face_num, "material").c_str(), obj, face_num);

			//
			//	effects (face colour, alpha)
			//
			LLTextureEntry *te = obj->getTE(face_num);
			LLColor4 colour = te->getColor();

			domEffect *effect = (domEffect*)effects->add("effect");
			effect->setId(llformat("%s-f%d-%s", geomID, face_num, "fx").c_str());

			daeElement *t = effect->add("profile_COMMON technique");
			t->setAttribute("sid", "common");

			domElement *phong = t->add("phong");
			phong->add("diffuse color")->setCharData(llformat("%f %f %f %f", colour.mV[0], colour.mV[1], colour.mV[2], colour.mV[3]).c_str());
			phong->add("transparency float")->setCharData(llformat("%f", colour.mV[3]).c_str());

			//
			//	materials
			//
			domMaterial *mat = (domMaterial*)materials->add("material");
			mat->setId(llformat("%s-f%d-%s", geomID, face_num, "material").c_str());
			domElement *matEffect = mat->add("instance_effect");
			matEffect->setAttribute("url", llformat("#%s-f%d-%s", geomID, face_num, "fx").c_str());
		}

		daeElement *node = scene->add("node");
		node->setAttribute("type", "NODE");
		node->setAttribute("id", geomID);
		node->setAttribute("name", geomID);

		//
		//	set tranform matrix (node position, rotation and scale)
		//
		domMatrix *matrix = (domMatrix*)node->add("matrix");
		LLXform srt;
		LLMatrix4 m4;

		srt.setScale(obj->getScale());
		srt.setPosition(obj->getRenderPosition() + mOffset);
		srt.setRotation(obj->getRenderRotation());
		srt.getLocalMat4(m4);

		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				(matrix->getValue()).append(m4.mMatrix[j][i]);
			}
		}

		//
		//	geometry of the node
		//
		daeElement *nodeGeometry = node->add("instance_geometry");

		//
		//	bind materials
		//
		daeElement *tq = nodeGeometry->add("bind_material technique_common");
		for (S32 face_num = 0; face_num < num_faces; face_num++) {
			daeElement *instanceMaterial = tq->add("instance_material");
			instanceMaterial->setAttribute("symbol", llformat("%s-f%d-%s", geomID, face_num, "material").c_str());
			instanceMaterial->setAttribute("target", llformat("#%s-f%d-%s", geomID, face_num, "material").c_str());
		}

		nodeGeometry->setAttribute("url", llformat("#%s-%s", geomID, "mesh").c_str());

	}
	root->add("scene instance_visual_scene")->setAttribute("url", "#Scene");

	return dae.writeAll();
}

DAESaver::DAESaver()
{}
