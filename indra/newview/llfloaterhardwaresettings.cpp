/** 
 * @file llfloaterhardwaresettings.cpp
 * @brief Menu of all the different graphics hardware settings
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterhardwaresettings.h"

// Viewer includes
#include "llfloaterpreference.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "llviewertexturelist.h"
#include "llfeaturemanager.h"
#include "llspinctrl.h"
#include "llstartup.h"
#include "lltextbox.h"
#include "llcombobox.h"
#include "pipeline.h"

// Linden library includes
#include "llradiogroup.h"
#include "lluictrlfactory.h"
#include "llwindow.h"
#include "llsliderctrl.h"

LLFloaterHardwareSettings::LLFloaterHardwareSettings(const LLSD& key)
	: LLFloater(key),

	  // these should be set on imminent refresh() call,
	  // but init them anyway
	  mUseVBO(0),
	  mUseAnisoSamples(0.f),
	  mFSAASamples(0),
	  mGamma(0.0),
	  mVideoCardMem(0),
	  mFogRatio(0.0),
	  mProbeHardwareOnStartup(FALSE)
{
}

LLFloaterHardwareSettings::~LLFloaterHardwareSettings()
{
}

void LLFloaterHardwareSettings::initCallbacks(void) 
{
	getChild<LLCheckBoxCtrl>("vbo")->setCommitCallback(boost::bind(&LLFloaterHardwareSettings::onRenderVBOEnableChange, this));
}

// menu maintenance functions

void LLFloaterHardwareSettings::refresh()
{
	LLPanel::refresh();

	mUseVBO = gSavedSettings.getBOOL("RenderVBOEnable");
	mUseAnisoSamples = gSavedSettings.getBOOL("RenderAnisotropic");
	mFSAASamples = gSavedSettings.getU32("RenderFSAASamples");
	mGamma = gSavedSettings.getF32("RenderGamma");
	mVideoCardMem = gSavedSettings.getS32("TextureMemory");
	mFogRatio = gSavedSettings.getF32("RenderFogRatio");
	mProbeHardwareOnStartup = gSavedSettings.getBOOL("ProbeHardwareOnStartup");
	mCompressTextures = gSavedSettings.getBOOL("RenderCompressTextures");

	getChild<LLUICtrl>("fsaa")->setValue((LLSD::Integer) mFSAASamples);
	getChild<LLUICtrl>("ani")->setValue((LLSD::Real)mUseAnisoSamples);
	refreshEnabledState();
}

void LLFloaterHardwareSettings::refreshEnabledState()
{
	F32 mem_multiplier = gSavedSettings.getF32("RenderTextureMemoryMultiple");
	S32Megabytes min_tex_mem = LLViewerTextureList::getMinVideoRamSetting();
	S32Megabytes max_tex_mem = LLViewerTextureList::getMaxVideoRamSetting(false, mem_multiplier);
	getChild<LLSliderCtrl>("GraphicsCardTextureMemory")->setMinValue(min_tex_mem.value());
	getChild<LLSliderCtrl>("GraphicsCardTextureMemory")->setMaxValue(max_tex_mem.value());

	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderVBOEnable") ||
		!gGLManager.mHasVertexBufferObject)
	{
		getChild<LLTextBox>("vbo_label")->setEnabled(FALSE);
		getChild<LLCheckBoxCtrl>("vbo")->setEnabled(FALSE);
		getChild<LLTextBox>("streamed_vbo_label")->setEnabled(FALSE);
		getChild<LLCheckBoxCtrl>("streamed_vbo")->setEnabled(FALSE);
	}
	else {
		onRenderVBOEnableChange();
	}

	if (!LLFeatureManager::getInstance()->isFeatureAvailable("RenderCompressTextures") ||
		!gGLManager.mHasVertexBufferObject)
	{
		getChildView("texture compression")->setEnabled(FALSE);
	}

	// anti-aliasing
	{
		LLUICtrl* fsaa_ctrl = getChild<LLUICtrl>("fsaa");
		LLTextBox* fsaa_text = getChild<LLTextBox>("antialiasing label");
		LLView* fsaa_restart = getChildView("antialiasing restart");
		
		// Enable or disable the control, the "Antialiasing:" label and the restart warning
		// based on code support for the feature on the current hardware.

		if (gPipeline.canUseAntiAliasing())
		{
			fsaa_ctrl->setEnabled(TRUE);
			fsaa_text->setEnabled(TRUE);

			fsaa_restart->setVisible(!gSavedSettings.getBOOL("RenderDeferred"));
		}
		else
		{
			fsaa_ctrl->setEnabled(FALSE);
			fsaa_ctrl->setValue((LLSD::Integer) 0);
			fsaa_text->setEnabled(FALSE);
			fsaa_restart->setVisible(FALSE);
		}
	}
}

//============================================================================

BOOL LLFloaterHardwareSettings::postBuild()
{
	childSetAction("OK", onBtnOK, this);
	childSetAction("Cancel", onBtnCancel, this);

// Don't do this on Mac as their braindead GL versioning
// sets this when 8x and 16x are indeed available
//
	if (gGLManager.mIsIntel || gGLManager.mGLVersion < 3.f)
	{ //remove FSAA settings above "4x"
		LLComboBox* combo = getChild<LLComboBox>("fsaa");
		combo->remove("8x");
		combo->remove("16x");
	}

	if (gPipeline.canUseWindLightShaders()) {
		//
		//	windlight shaders are available so hide the gamma and
		//	fog distance spinners
		//
		LLSpinCtrl *fog = getChild<LLSpinCtrl>("fog");
		LLSpinCtrl *gamma = getChild<LLSpinCtrl>("gamma");

		fog->setVisible(FALSE);
		gamma->setVisible(FALSE);

		getChild<LLTextBox>("gamma_note")->setVisible(FALSE);

		//
		//	reshape the floater to get rid of the blank space
		//	caused by hiding the gamma and fog distance spinners
		//
		reshape(getRect().getWidth(), getRect().getHeight() - fog->getRect().getHeight() - gamma->getRect().getHeight() - 5);
	}
	else {
		//
		//	no windlight shaders are available so enable the
		//	gamma and fog distance
		//
		getChild<LLSpinCtrl>("fog")->setEnabled(TRUE);
		getChild<LLSpinCtrl>("gamma")->setEnabled(TRUE);
		getChild<LLTextBox>("gamma_note")->setEnabled(TRUE);
	}

	refresh();
	center();

	// load it up
	initCallbacks();
	return TRUE;
}


void LLFloaterHardwareSettings::apply()
{
	refresh();
}


void LLFloaterHardwareSettings::cancel()
{
	closeFloater();
}

void LLFloaterHardwareSettings::onRenderVBOEnableChange()
{
	const BOOL enable = gSavedSettings.getBOOL("RenderVBOEnable");

	getChild<LLTextBox>("streamed_vbo_label")->setEnabled(enable);
	getChild<LLCheckBoxCtrl>("streamed_vbo")->setEnabled(enable);
}

// static 
void LLFloaterHardwareSettings::onBtnCancel( void* userdata )
{
	LLFloaterHardwareSettings *fp =(LLFloaterHardwareSettings *)userdata;
	fp->cancel();
}

// static
void LLFloaterHardwareSettings::onBtnOK( void* userdata )
{
	LLFloaterHardwareSettings *fp =(LLFloaterHardwareSettings *)userdata;
	fp->apply();
	fp->closeFloater(false);
}

void LLFloaterHardwareSettings::onClose(bool app_quitting)
{
	gSavedSettings.setBOOL("RenderVBOEnable", mUseVBO);
	gSavedSettings.setBOOL("RenderAnisotropic", mUseAnisoSamples);
	gSavedSettings.setU32("RenderFSAASamples", mFSAASamples);
	gSavedSettings.setF32("RenderGamma", mGamma);
	gSavedSettings.setS32("TextureMemory", mVideoCardMem);
	gSavedSettings.setF32("RenderFogRatio", mFogRatio);
	gSavedSettings.setBOOL("ProbeHardwareOnStartup", mProbeHardwareOnStartup );
	gSavedSettings.setBOOL("RenderCompressTextures", mCompressTextures );
}
