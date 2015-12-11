/** 
 * @file lloutfitgallery.cpp
 * @author Pavlo Kryvych
 * @brief Visual gallery of agent's outfits for My Appearance side panel
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2015, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h" // must be first include
#include "lloutfitgallery.h"

#include <boost/foreach.hpp>

// llcommon
#include "llcommonutils.h"
#include "llvfile.h"

#include "llappearancemgr.h"
#include "lleconomy.h"
#include "llerror.h"
#include "llfilepicker.h"
#include "llfloaterperms.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "lllocalbitmaps.h"
#include "llviewermenufile.h"
#include "llwearableitemslist.h"


static LLPanelInjector<LLOutfitGallery> t_outfit_gallery("outfit_gallery");
static LLOutfitGallery* gOutfitGallery = NULL;

LLOutfitGallery::LLOutfitGallery()
    : LLOutfitListBase(),
      mTexturesObserver(NULL)
{
//    mGearMenu = new LLOutfitGalleryGearMenu(this);
}

BOOL LLOutfitGallery::postBuild()
{
    BOOL rv = LLOutfitListBase::postBuild();
    return rv;
}

void LLOutfitGallery::onOpen(const LLSD& info)
{
    LLOutfitListBase::onOpen(info);
    rebuildGallery();
    loadPhotos();
}

#define LAYOUT_STACK_HEIGHT 180
#define GALLERY_VERTICAL_GAP 10
#define GALLERY_HORIZONTAL_GAP 10
#define GALLERY_ITEM_WIDTH 150
#define GALLERY_ITEM_HEIGHT 175
#define GALLERY_ITEM_HGAP 16 
#define ITEMS_IN_ROW 3
#define LAYOUT_STACK_WIDTH 166 * ITEMS_IN_ROW//498
#define GALLERY_WIDTH 163 * ITEMS_IN_ROW//485//290

void LLOutfitGallery::rebuildGallery()
{
    LLView::child_list_t child_list(*getChild<LLPanel>("gallery_panel")->getChildList());
    BOOST_FOREACH(LLView* view, child_list)
    {
        LLLayoutStack* lstack = dynamic_cast<LLLayoutStack*>(view);
        if (lstack != NULL)
        {
            LLView::child_list_t panel_list(*lstack->getChildList());
            BOOST_FOREACH(LLView* panel, panel_list)
            {
                //LLLayoutPanel* lpanel = dynamic_cast<LLLayoutPanel*>(panel);
                //if (!lpanel)
                //    continue;
                LLView::child_list_t panel_children(*panel->getChildList());
                if (panel_children.size() > 0)
                {
                    //Assume OutfitGalleryItem is the only one child of layout panel
                    LLView* view_item = panel_children.back();
                    LLOutfitGalleryItem* gallery_item = dynamic_cast<LLOutfitGalleryItem*>(view_item);
                    if (gallery_item != NULL)
                        panel->removeChild(gallery_item);
                }
                lstack->removeChild(panel);
                //delete panel;
            }
            getChild<LLPanel>("gallery_panel")->removeChild(lstack);
            delete lstack;
        }
        else
        {
            getChild<LLPanel>("gallery_panel")->removeChild(view);
        }
    }
    uuid_vec_t cats;
    getCurrentCategories(cats);
    int n = cats.size();
    int row_count = (n % ITEMS_IN_ROW) == 0 ? n / ITEMS_IN_ROW : n / ITEMS_IN_ROW + 1;
    getChild<LLPanel>("gallery_panel")->reshape(
        GALLERY_WIDTH, row_count * (LAYOUT_STACK_HEIGHT + GALLERY_VERTICAL_GAP));
    int vgap = 0;
    uuid_vec_t::reverse_iterator rit = cats.rbegin();
    for (int i = row_count - 1; i >= 0; i--)
    {
        LLLayoutStack* stack = buildLayoutStak(0, (row_count - 1 - i) * LAYOUT_STACK_HEIGHT + vgap);
        getChild<LLPanel>("gallery_panel")->addChild(stack);
        int items_in_cur_row = (n % ITEMS_IN_ROW) == 0 ? ITEMS_IN_ROW : (i == row_count - 1 ? n % ITEMS_IN_ROW : ITEMS_IN_ROW);
        int hgap = 0;
        for (int j = 0; j < items_in_cur_row && rit != cats.rend(); j++)
        {
            LLLayoutPanel* lpanel = buildLayoutPanel(j * GALLERY_ITEM_WIDTH + hgap);
            LLOutfitGalleryItem* item = mOutfitMap[*rit];
            lpanel->addChild(item);
            stack->addChild(lpanel);
            rit++;
            hgap += GALLERY_HORIZONTAL_GAP;
        }
        vgap += GALLERY_VERTICAL_GAP;
    }
}

LLOutfitGalleryItem* LLOutfitGallery::buildGalleryItem(std::string name)
{
    LLOutfitGalleryItem::Params giparams;
    LLOutfitGalleryItem* gitem = LLUICtrlFactory::create<LLOutfitGalleryItem>(giparams);
    LLRect girect = LLRect(0, GALLERY_ITEM_HEIGHT - GALLERY_ITEM_HEIGHT,
        GALLERY_ITEM_WIDTH, 0);
    //gitem->setRect(girect);
    gitem->reshape(GALLERY_ITEM_WIDTH, GALLERY_ITEM_HEIGHT);
    gitem->setVisible(true);
    gitem->setFollowsLeft();
    gitem->setFollowsTop();
    gitem->setOutfitName(name);
    return gitem;
}

LLLayoutPanel* LLOutfitGallery::buildLayoutPanel(int left)
{
    LLLayoutPanel::Params lpparams;
    int top = 0;
    LLLayoutPanel* lpanel = LLUICtrlFactory::create<LLLayoutPanel>(lpparams);
    LLRect rect = LLRect(left, top + GALLERY_ITEM_HEIGHT, left + GALLERY_ITEM_WIDTH + GALLERY_ITEM_HGAP, top);
    lpanel->setRect(rect);
    lpanel->reshape(GALLERY_ITEM_WIDTH + GALLERY_ITEM_HGAP, GALLERY_ITEM_HEIGHT);
    lpanel->setVisible(true);
    lpanel->setFollowsLeft();
    lpanel->setFollowsTop();
    return lpanel;
}

LLLayoutStack* LLOutfitGallery::buildLayoutStak(int left, int top)
{
    LLLayoutStack::Params sparams;
    LLLayoutStack* stack = LLUICtrlFactory::create<LLLayoutStack>(sparams);
    LLRect rect = LLRect(left, top + LAYOUT_STACK_HEIGHT, left + LAYOUT_STACK_WIDTH, top);
    stack->setRect(rect);
    stack->reshape(LAYOUT_STACK_WIDTH, LAYOUT_STACK_HEIGHT);
    stack->setVisible(true);
    stack->setFollowsLeft();
    stack->setFollowsTop();
    return stack;
}


LLOutfitGallery::~LLOutfitGallery()
{
    if (gInventory.containsObserver(mTexturesObserver))
    {
        gInventory.removeObserver(mTexturesObserver);
    }
    delete mTexturesObserver;
}

void LLOutfitGallery::setFilterSubString(const std::string& string)
{
    //TODO: Implement filtering

    sFilterSubString = string;
}

void LLOutfitGallery::onHighlightBaseOutfit(LLUUID base_id, LLUUID prev_id)
{
    if (mOutfitMap[base_id])
    {
        mOutfitMap[base_id]->setOutfitWorn(true);
    }
    if (mOutfitMap[prev_id])
    {
        mOutfitMap[prev_id]->setOutfitWorn(false);
    }
}

void LLOutfitGallery::onSetSelectedOutfitByUUID(const LLUUID& outfit_uuid)
{
}

void LLOutfitGallery::getCurrentCategories(uuid_vec_t& vcur)
{
    for (outfit_map_t::const_iterator iter = mOutfitMap.begin();
        iter != mOutfitMap.end();
        iter++)
    {
        if ((*iter).second != NULL)
        {
            vcur.push_back((*iter).first);
        }
    }
}

void LLOutfitGallery::updateAddedCategory(LLUUID cat_id)
{
    LLViewerInventoryCategory *cat = gInventory.getCategory(cat_id);
    if (!cat) return;

    std::string name = cat->getName();
    LLOutfitGalleryItem* item = buildGalleryItem(name);
    mOutfitMap.insert(LLOutfitGallery::outfit_map_value_t(cat_id, item));
    item->setRightMouseDownCallback(boost::bind(&LLOutfitListBase::outfitRightClickCallBack, this,
        _1, _2, _3, cat_id));
    LLWearableItemsList* list = NULL;
    item->setFocusReceivedCallback(boost::bind(&LLOutfitListBase::˝hangeOutfitSelection, this, list, cat_id));

}

void LLOutfitGallery::updateRemovedCategory(LLUUID cat_id)
{
    outfit_map_t::iterator outfits_iter = mOutfitMap.find(cat_id);
    if (outfits_iter != mOutfitMap.end())
    {
        //const LLUUID& outfit_id = outfits_iter->first;
        LLOutfitGalleryItem* item = outfits_iter->second;

        // An outfit is removed from the list. Do the following:
        // 2. Remove the outfit from selection.
        //deselectOutfit(outfit_id);

        // 3. Remove category UUID to accordion tab mapping.
        mOutfitMap.erase(outfits_iter);

        // 4. Remove outfit from gallery.
        rebuildGallery();

        // kill removed item
        if (item != NULL)
        {
            item->die();
        }
    }

}

void LLOutfitGallery::updateChangedCategoryName(LLViewerInventoryCategory *cat, std::string name)
{
    outfit_map_t::iterator outfit_iter = mOutfitMap.find(cat->getUUID());
    if (outfit_iter != mOutfitMap.end())
    {
        // Update name of outfit in gallery
        LLOutfitGalleryItem* item = outfit_iter->second;
        if (item)
        {
            item->setOutfitName(name);
        }
    }
}

void LLOutfitGallery::onOutfitRightClick(LLUICtrl* ctrl, S32 x, S32 y, const LLUUID& cat_id)
{
    if (mOutfitMenu && cat_id.notNull())
    {
        uuid_vec_t selected_uuids;
        selected_uuids.push_back(cat_id);
        mOutfitMenu->show(ctrl, selected_uuids, x, y);
    }
}

void LLOutfitGallery::onChangeOutfitSelection(LLWearableItemsList* list, const LLUUID& category_id)
{
    if (mSelectedOutfitUUID == category_id)
        return;
    if (mOutfitMap[mSelectedOutfitUUID])
    {
        mOutfitMap[mSelectedOutfitUUID]->setSelected(FALSE);
    }
    if (mOutfitMap[category_id])
    {
        mOutfitMap[category_id]->setSelected(TRUE);
    }
}

bool LLOutfitGallery::hasItemSelected()
{
    return false;
}

bool LLOutfitGallery::canWearSelected()
{
    return false;
}

LLOutfitListGearMenuBase* LLOutfitGallery::createGearMenu()
{
    return new LLOutfitGalleryGearMenu(this);
}

static LLDefaultChildRegistry::Register<LLOutfitGalleryItem> r("outfit_gallery_item");

LLOutfitGalleryItem::LLOutfitGalleryItem(const Params& p)
    : LLPanel(p),
    mTexturep(NULL)
{
    buildFromFile("panel_outfit_gallery_item.xml");
}

LLOutfitGalleryItem::~LLOutfitGalleryItem()
{

}

BOOL LLOutfitGalleryItem::postBuild()
{
    setDefaultImage();

    mOutfitNameText = getChild<LLTextBox>("outfit_name");
    mOutfitWornText = getChild<LLTextBox>("outfit_worn_text");
    setOutfitWorn(false);
    return TRUE;
}

void LLOutfitGalleryItem::draw()
{
    LLPanel::draw();

    // In case texture is not set, don't draw it over default image
    if (!mTexturep)
    {
        return;
    }

    // Border
    LLRect border = getChildView("preview_outfit")->getRect();
    //gl_rect_2d(border, LLColor4::black, FALSE);


    // Interior
    LLRect interior = border;
    //interior.stretch(-1);

    // If the floater is focused, don't apply its alpha to the texture (STORM-677).
    const F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();
    if (mTexturep)
    {
        if (mTexturep->getComponents() == 4)
        {
            gl_rect_2d_checkerboard(interior, alpha);
        }

        gl_draw_scaled_image(interior.mLeft, interior.mBottom, interior.getWidth(), interior.getHeight(), mTexturep, UI_VERTEX_COLOR % alpha);

        // Pump the priority
        mTexturep->addTextureStats((F32)(interior.getWidth() * interior.getHeight()));
    }
}

void LLOutfitGalleryItem::setOutfitName(std::string name)
{
    mOutfitNameText->setText(name);
}

void LLOutfitGalleryItem::setOutfitWorn(bool value)
{
    //LLStringUtil::format_map_t string_args;
    //std::string worn_text = getString("worn_text", string_args);

    if (value)
    {
        mOutfitWornText->setValue("(worn)");
    }
    else
    {
        mOutfitWornText->setValue("");
    }
}

void LLOutfitGalleryItem::setSelected(bool value)
{
    if (value)
    {
        mOutfitWornText->setValue("(selected)");
    }
    else
    {
        mOutfitWornText->setValue("");
    }
}

BOOL LLOutfitGalleryItem::handleMouseDown(S32 x, S32 y, MASK mask)
{
    setFocus(TRUE);
    return LLUICtrl::handleMouseDown(x, y, mask);
}

void LLOutfitGalleryItem::setImageAssetId(LLUUID image_asset_id)
{
    mTexturep = LLViewerTextureManager::getFetchedTexture(image_asset_id);
    mTexturep->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
}

void LLOutfitGalleryItem::setDefaultImage()
{
    /*
    LLUUID imageAssetID("e417f443-a199-bac1-86b0-0530e177fb54");
    mTexturep = LLViewerTextureManager::getFetchedTexture(imageAssetID);
    mTexturep->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
    */
    mTexturep = NULL;
}

LLOutfitGalleryGearMenu::LLOutfitGalleryGearMenu(LLOutfitListBase* olist)
    : LLOutfitListGearMenuBase(olist)
{
}

void LLOutfitGalleryGearMenu::onUpdateItemsVisibility()
{
    if (!mMenu) return;
    mMenu->setItemVisible("expand", FALSE);
    mMenu->setItemVisible("collapse", FALSE);
    mMenu->setItemVisible("upload_foto", TRUE);
    mMenu->setItemVisible("load_assets", TRUE);
    LLOutfitListGearMenuBase::onUpdateItemsVisibility();
}

void LLOutfitGalleryGearMenu::onUploadFoto()
{
    LLUUID selected_outfit_id = getSelectedOutfitID();
    LLOutfitGallery* gallery = dynamic_cast<LLOutfitGallery*>(mOutfitList);
    if (gallery && selected_outfit_id.notNull())
    {
        gallery->uploadPhoto(selected_outfit_id);
    }
    if (selected_outfit_id.notNull())
    {

    }
}

void LLOutfitGalleryGearMenu::onLoadAssets()
{
    LLOutfitGallery* gallery = dynamic_cast<LLOutfitGallery*>(mOutfitList);
    if (gallery != NULL)
    {
        gallery->loadPhotos();
    }
}

void LLOutfitGallery::loadPhotos()
{
    //Iterate over inventory
    LLUUID textures = gInventory.findCategoryUUIDForType(LLFolderType::EType::FT_TEXTURE);
    LLViewerInventoryCategory* category = gInventory.getCategory(textures);
    if (!category)
        return;

    if (mTexturesObserver == NULL)
    {
        mTexturesObserver = new LLInventoryCategoriesObserver();
        gInventory.addObserver(mTexturesObserver);
    }

    // Start observing changes in "My Outfits" category.
    mTexturesObserver->addCategory(textures,
        boost::bind(&LLOutfitGallery::refreshTextures, this, textures));

    category->fetch();
    refreshTextures(textures);
}

void LLOutfitGallery::refreshTextures(const LLUUID& category_id)
{
    LLInventoryModel::cat_array_t cat_array;
    LLInventoryModel::item_array_t item_array;

    // Collect all sub-categories of a given category.
    LLIsType is_texture(LLAssetType::AT_TEXTURE);
    gInventory.collectDescendentsIf(
        category_id,
        cat_array,
        item_array,
        LLInventoryModel::EXCLUDE_TRASH,
        is_texture);

    //Find textures which contain outfit UUID string in description
    LLInventoryModel::item_array_t uploaded_item_array;
    BOOST_FOREACH(LLViewerInventoryItem* item, item_array)
    {
        std::string desc = item->getDescription();

        LLUUID outfit_id(desc);

        //Check whether description contains correct UUID of outfit
        if (outfit_id.isNull())
            continue;

        outfit_map_t::iterator outfit_it = mOutfitMap.find(outfit_id);
        if (outfit_it != mOutfitMap.end() && !outfit_it->first.isNull())
        {
            uploaded_item_array.push_back(item);
        }
    }

    uuid_vec_t vadded;
    uuid_vec_t vremoved;

    // Create added and removed items vectors.
    computeDifferenceOfTextures(uploaded_item_array, vadded, vremoved);

    BOOST_FOREACH(LLUUID item_id, vadded)
    {
        LLViewerInventoryItem* added_item = gInventory.getItem(item_id);
        LLUUID asset_id = added_item->getAssetUUID();
        std::string desc = added_item->getDescription();
        LLUUID outfit_id(desc);
        mOutfitMap[outfit_id]->setImageAssetId(asset_id);
        mTextureMap[outfit_id] = added_item;
    }

    BOOST_FOREACH(LLUUID item_id, vremoved)
    {
        LLViewerInventoryItem* rm_item = gInventory.getItem(item_id);
        std::string desc = rm_item->getDescription();
        LLUUID outfit_id(desc);
        mOutfitMap[outfit_id]->setDefaultImage();
        mTextureMap.erase(outfit_id);
    }

    /*
    LLInventoryModel::item_array_t::const_iterator it = item_array.begin();
    for ( ; it != item_array.end(); it++)
    {
        LLViewerInventoryItem* item = (*it);
        LLUUID asset_id = item->getAssetUUID();

        std::string desc = item->getDescription();

        LLUUID outfit_id(desc);

        //Check whether description contains correct UUID of outfit
        if (outfit_id.isNull())
            continue;

        outfit_map_t::iterator outfit_it = mOutfitMap.find(outfit_id);
        if (outfit_it != mOutfitMap.end() && !outfit_it->first.isNull())
        {
            outfit_it->second->setImageAssetId(asset_id);
            mTextureMap[outfit_id] = item;
        }

        
        //LLUUID* item_idp = new LLUUID();

        //gOutfitGallery = this;
        //const BOOL high_priority = TRUE;
        //gAssetStorage->getAssetData(asset_id,
        //    LLAssetType::AT_TEXTURE,
        //    onLoadComplete,
        //    (void**)item_idp,
        //    high_priority);
        //
        ////mAssetStatus = PREVIEW_ASSET_LOADING;
        
    }
    */
}

void LLOutfitGallery::onLoadComplete(LLVFS *vfs, const LLUUID& asset_uuid, LLAssetType::EType type, void* user_data, S32 status, LLExtStat ext_status)
{
    LL_WARNS() << "asset_uuid: " << asset_uuid.asString() << LL_ENDL;

    LLUUID* outfit_id = (LLUUID*)user_data;
        if (!user_data)
        return;
    LL_WARNS() << "outfit_id: " << outfit_id->asString() << LL_ENDL;

    outfit_map_t::iterator it = gOutfitGallery->mOutfitMap.find(*outfit_id);
    if (it != gOutfitGallery->mOutfitMap.end() && !it->first.isNull())
    {
    }
}

void LLOutfitGallery::uploadPhoto(LLUUID outfit_id)
{
    outfit_map_t::iterator outfit_it = mOutfitMap.find(outfit_id);
    if (outfit_it == mOutfitMap.end() || outfit_it->first.isNull())
    {
        return;
    }

    bool add_successful = false;
    LLFilePicker& picker = LLFilePicker::instance();
    if (picker.getOpenFile(LLFilePicker::FFLOAD_IMAGE))
    {
        std::string filename = picker.getFirstFile();
        LLLocalBitmap* unit = new LLLocalBitmap(filename);
        if (unit->getValid())
        {
            add_successful = true;
            LLAssetStorage::LLStoreAssetCallback callback = NULL;
            S32 expected_upload_cost = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload(); // kinda hack - assumes that unsubclassed LLFloaterNameDesc is only used for uploading chargeable assets, which it is right now (it's only used unsubclassed for the sound upload dialog, and THAT should be a subclass).
            void *nruserdata = NULL;
            nruserdata = (void *)&outfit_id;

            LL_WARNS() << "selected_outfit_id: " << outfit_id.asString() << LL_ENDL;

            //LLViewerInventoryItem *outfit = gInventory.getItem(selected_outfit_id);
            LLViewerInventoryCategory *outfit_cat = gInventory.getCategory(outfit_id);
            if (!outfit_cat) return;

            checkRemovePhoto(outfit_id);

            LLStringUtil::format_map_t foto_string_args;
            foto_string_args["OUTFIT_NAME"] = outfit_cat->getName();
            std::string display_name = getString("outfit_foto_string", foto_string_args);

            upload_new_resource(filename, // file
                display_name,
                outfit_id.asString(),
                0, LLFolderType::FT_NONE, LLInventoryType::IT_NONE,
                LLFloaterPerms::getNextOwnerPerms("Uploads"),
                LLFloaterPerms::getGroupPerms("Uploads"),
                LLFloaterPerms::getEveryonePerms("Uploads"),
                display_name, callback, expected_upload_cost, nruserdata);
        }
    }
}

bool LLOutfitGallery::checkRemovePhoto(LLUUID outfit_id)
{
    //remove existing photo of outfit from inventory
    texture_map_t::iterator texture_it = mTextureMap.find(outfit_id);
    if (texture_it != mTextureMap.end()) {
        gInventory.removeItem(texture_it->second->getUUID());
        return true;
    }
    return false;
}

void LLOutfitGallery::setUploadedPhoto(LLUUID outfit_id, LLUUID asset_id)
{

}

void LLOutfitGallery::computeDifferenceOfTextures(
    const LLInventoryModel::item_array_t& vtextures,
    uuid_vec_t& vadded,
    uuid_vec_t& vremoved)
{
    uuid_vec_t vnew;
    // Creating a vector of newly collected sub-categories UUIDs.
    for (LLInventoryModel::item_array_t::const_iterator iter = vtextures.begin();
        iter != vtextures.end();
        iter++)
    {
        vnew.push_back((*iter)->getUUID());
    }

    uuid_vec_t vcur;
    // Creating a vector of currently uploaded texture UUIDs.
    for (texture_map_t::const_iterator iter = mTextureMap.begin();
        iter != mTextureMap.end();
        iter++)
    {
        vcur.push_back((*iter).second->getUUID());
    }
//    getCurrentCategories(vcur);

    LLCommonUtils::computeDifference(vnew, vcur, vadded, vremoved);

}
