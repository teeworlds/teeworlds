#include "editor2.h"

void CEditor2::EditDeleteLayer(int LyID, int ParentGroupID)
{
	dbg_assert(m_Map.m_aLayers.IsValid(LyID), "LyID out of bounds");
	dbg_assert(m_Map.m_aGroups.IsValid(ParentGroupID), "ParentGroupID out of bounds");
	dbg_assert(LyID != m_Map.m_GameLayerID, "Can't delete game layer");
	dbg_assert(m_Map.m_aLayers.Count() > 0, "There should be at least a game layer");

	char aHistoryDesc[64];
	str_format(aHistoryDesc, sizeof(aHistoryDesc), "%s", GetLayerName(LyID));

	CEditorMap2::CGroup& ParentGroup = m_Map.m_aGroups.Get(ParentGroupID);
	const int ParentGroupLayerCount = ParentGroup.m_LayerCount;
	dbg_assert(ParentGroupLayerCount > 0, "Parent group layer count is zero?");

	// remove layer id from parent group
	int GroupLayerID = -1;
	for(int li = 0; li < ParentGroupLayerCount && GroupLayerID == -1; li++)
	{
		if(ParentGroup.m_apLayerIDs[li] == LyID)
			GroupLayerID = li;
	}
	dbg_assert(GroupLayerID != -1, "Layer not found inside parent group");

	memmove(&ParentGroup.m_apLayerIDs[GroupLayerID], &ParentGroup.m_apLayerIDs[GroupLayerID+1],
			(ParentGroup.m_LayerCount-GroupLayerID) * sizeof(ParentGroup.m_apLayerIDs[0]));
	ParentGroup.m_LayerCount--;

	// delete actual layer
	m_Map.m_aLayers.RemoveByID(LyID);

	// validation checks
#ifdef CONF_DEBUG
	dbg_assert(m_Map.m_aLayers.IsValid(m_Map.m_GameLayerID), "m_Map.m_GameLayerID is invalid");
	dbg_assert(m_Map.m_aGroups.IsValid(m_Map.m_GameGroupID), "m_UiSelectedGroupID is invalid");

	const int GroupCount = m_Map.m_aGroups.Count();
	const CEditorMap2::CGroup* aGroups = m_Map.m_aGroups.Data();
	for(int gi = 0; gi < GroupCount; gi++)
	{
		const CEditorMap2::CGroup& Group = aGroups[gi];
		const int LayerCount = Group.m_LayerCount;
		for(int li = 0; li < LayerCount; li++)
		{
			dbg_assert(m_Map.m_aLayers.IsValid(Group.m_apLayerIDs[li]), "Group.m_apLayerIDs[li] is invalid");
		}
	}
#endif

	// history entry
	HistoryNewEntry("Deleted layer", aHistoryDesc);
}

void CEditor2::EditDeleteGroup(u32 GroupID)
{
	dbg_assert(m_Map.m_aGroups.IsValid(GroupID), "GroupID out of bounds");
	dbg_assert(GroupID != (u32)m_Map.m_GameGroupID, "Can't delete game group");

	CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(GroupID);
	while(Group.m_LayerCount > 0)
	{
		EditDeleteLayer(Group.m_apLayerIDs[0], GroupID);
	}

	// find list index
	const int Count = m_Map.m_GroupIDListCount;
	int GroupListIndex = -1;
	for(int i = 0; i < Count; i++)
	{
		if(m_Map.m_aGroupIDList[i] == GroupID)
		{
			GroupListIndex = i;
			break;
		}
	}

	dbg_assert(GroupListIndex != -1, "Group not found");

	// remove from list
	if(GroupListIndex < Count-1)
		mem_move(&m_Map.m_aGroupIDList[GroupListIndex], &m_Map.m_aGroupIDList[GroupListIndex+1], (Count - GroupListIndex -1) * sizeof(m_Map.m_aGroupIDList[0]));

	m_Map.m_GroupIDListCount--;

	// remove group
	m_Map.m_aGroups.RemoveByID(GroupID);

	// history entry
	char aBuff[64];
	str_format(aBuff, sizeof(aBuff), "Group %d", GroupID);
	HistoryNewEntry("Deleted group", aBuff);
}

void CEditor2::EditDeleteImage(int ImgID)
{
	dbg_assert(ImgID >= 0 && ImgID <= m_Map.m_Assets.m_ImageCount, "ImgID out of bounds");

	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "%s", m_Map.m_Assets.m_aImageNames[ImgID].m_Buff);

	m_Map.AssetsDeleteImage(ImgID);

	// history entry
	HistoryNewEntry("Deleted image", aHistoryEntryDesc);
}

void CEditor2::EditAddImage(const char* pFilename)
{
	bool Success = m_Map.AssetsAddAndLoadImage(pFilename);

	// history entry
	if(Success)
	{
		char aHistoryEntryDesc[64];
		str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "%s", pFilename);
		HistoryNewEntry("Added image", aHistoryEntryDesc);
	}
}

void CEditor2::EditCreateAndAddGroup()
{
	dbg_assert(m_Map.m_GroupIDListCount < CEditorMap2::MAX_GROUPS, "Group list is full");

	CEditorMap2::CGroup Group;
	Group.m_OffsetX = 0;
	Group.m_OffsetY = 0;
	Group.m_ParallaxX = 100;
	Group.m_ParallaxY = 100;
	Group.m_LayerCount = 0;
	u32 GroupID = m_Map.m_aGroups.Push(Group);
	m_Map.m_aGroupIDList[m_Map.m_GroupIDListCount++] = GroupID;

	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "Group %d", GroupID);
	HistoryNewEntry("New group", aHistoryEntryDesc);
}

int CEditor2::EditCreateAndAddTileLayerUnder(int UnderLyID, int GroupID)
{
	dbg_assert(m_Map.m_aGroups.IsValid(GroupID), "GroupID out of bounds");

	// base width and height on given layer if it's a tilelayer, else base on game layer
	int LyWidth = m_Map.m_aLayers.Get(m_Map.m_GameLayerID).m_Width;
	int LyHeight = m_Map.m_aLayers.Get(m_Map.m_GameLayerID).m_Height;
	int LyImageID = -1;

	CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(GroupID);

	int UnderGrpLyID = -1;
	if(UnderLyID != -1)
	{
		dbg_assert(m_Map.m_aLayers.IsValid(UnderLyID), "LyID out of bounds");
		const CEditorMap2::CLayer& TopLayer = m_Map.m_aLayers.Get(UnderLyID);

		if(TopLayer.IsTileLayer())
		{
			LyWidth = TopLayer.m_Width;
			LyHeight = TopLayer.m_Height;
			LyImageID = TopLayer.m_ImageID;
		}

		const int ParentGroupLayerCount = Group.m_LayerCount;
		for(int li = 0; li < ParentGroupLayerCount; li++)
		{
			if(Group.m_apLayerIDs[li] == UnderLyID)
			{
				UnderGrpLyID = li;
				break;
			}
		}

		dbg_assert(UnderGrpLyID != -1, "Layer not found in parent group");
	}

	dbg_assert(Group.m_LayerCount < CEditorMap2::MAX_GROUP_LAYERS, "Group is full of layers");

	u32 NewLyID;
	CEditorMap2::CLayer& Layer = m_Map.NewTileLayer(LyWidth, LyHeight, &NewLyID);
	Layer.m_ImageID = LyImageID;

	const int GrpLyID = UnderGrpLyID+1;
	mem_move(&Group.m_apLayerIDs[GrpLyID+1], &Group.m_apLayerIDs[GrpLyID], (Group.m_LayerCount-GrpLyID) * sizeof(Group.m_apLayerIDs[0]));

	Group.m_apLayerIDs[GrpLyID] = NewLyID;
	Group.m_LayerCount++;

	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "Tile %d", NewLyID);
	HistoryNewEntry(Localize("New tile layer"), aHistoryEntryDesc);
	return NewLyID;
}

int CEditor2::EditCreateAndAddQuadLayerUnder(int UnderLyID, int GroupID)
{
	dbg_assert(m_Map.m_aLayers.IsValid(UnderLyID), "LyID out of bounds");
	dbg_assert(m_Map.m_aGroups.IsValid(GroupID), "GroupID out of bounds");

	u32 NewLayerID;
	m_Map.NewQuadLayer(&NewLayerID);
	CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(GroupID);

	int UnderGrpLyID = -1;
	const int ParentGroupLayerCount = Group.m_LayerCount;
	for(int li = 0; li < ParentGroupLayerCount; li++)
	{
		if(Group.m_apLayerIDs[li] == UnderLyID)
		{
			UnderGrpLyID = li;
			break;
		}
	}

	dbg_assert(UnderGrpLyID != -1, "Layer not found in parent group");
	dbg_assert(Group.m_LayerCount < CEditorMap2::MAX_GROUP_LAYERS, "Group is full of layers");

	const int GrpLyID = UnderGrpLyID+1;
	memmove(&Group.m_apLayerIDs[GrpLyID+1], &Group.m_apLayerIDs[GrpLyID],
		(Group.m_LayerCount-GrpLyID) * sizeof(Group.m_apLayerIDs[0]));
	Group.m_LayerCount++;

	Group.m_apLayerIDs[GrpLyID] = NewLayerID;

	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "Quad %d", NewLayerID);
	HistoryNewEntry(Localize("New Quad layer"), aHistoryEntryDesc);
	return NewLayerID;
}

void CEditor2::EditLayerChangeImage(int LayerID, int NewImageID)
{
	dbg_assert(m_Map.m_aLayers.IsValid(LayerID), "LayerID out of bounds");
	dbg_assert(NewImageID >= -1 && NewImageID < m_Map.m_Assets.m_ImageCount, "NewImageID out of bounds");

	const int OldImageID = m_Map.m_aLayers.Get(LayerID).m_ImageID;
	if(OldImageID == NewImageID)
		return;

	m_Map.m_aLayers.Get(LayerID).m_ImageID = NewImageID;

	char aHistoryEntryAction[64];
	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("%s: changed image"),
		GetLayerName(LayerID));
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "%s > %s",
		OldImageID < 0 ? Localize("none") : m_Map.m_Assets.m_aImageNames[OldImageID].m_Buff,
		NewImageID < 0 ? Localize("none") : m_Map.m_Assets.m_aImageNames[NewImageID].m_Buff);
	HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
}

void CEditor2::EditLayerHighDetail(int LayerID, bool NewHighDetail)
{
	dbg_assert(m_Map.m_aLayers.IsValid(LayerID), "LayerID out of bounds");

	CEditorMap2::CLayer& Layer = m_Map.m_aLayers.Get(LayerID);
	if(Layer.m_HighDetail == NewHighDetail)
		return;

	const bool OldHighDetail = Layer.m_HighDetail;
	Layer.m_HighDetail = NewHighDetail;

	char aHistoryEntryAction[64];
	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Layer %d: high detail"), LayerID);
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "%s > %s",
		OldHighDetail ? Localize("on") : Localize("off"),
		NewHighDetail ? Localize("on") : Localize("off"));
	HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
}

void CEditor2::EditGroupUseClipping(int GroupID, bool NewUseClipping)
{
	dbg_assert(m_Map.m_aGroups.IsValid(GroupID), "GroupID out of bounds");

	CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(GroupID);
	if(Group.m_UseClipping == NewUseClipping)
		return;

	const bool OldUseClipping = Group.m_UseClipping;
	Group.m_UseClipping = NewUseClipping;

	char aHistoryEntryAction[64];
	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Group %d: use clipping"),
		GroupID);
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "%s > %s",
		OldUseClipping ? Localize("true") : Localize("false"),
		NewUseClipping ? Localize("true") : Localize("false"));
	HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
}

int CEditor2::EditGroupOrderMove(int GroupListIndex, int RelativePos)
{
	// returns new List Index

	dbg_assert(GroupListIndex >= 0 && GroupListIndex < m_Map.m_GroupIDListCount, "GroupListIndex out of bounds");
	dbg_assert(m_Map.m_aGroups.IsValid(m_Map.m_aGroupIDList[GroupListIndex]), "Group is invalid");

	const int OldGroupIndex = GroupListIndex;
	const int NewGroupListIndex = clamp(GroupListIndex + RelativePos, 0, m_Map.m_GroupIDListCount-1);
	if(NewGroupListIndex == GroupListIndex)
		return GroupListIndex;

	const int MoveDir = sign(RelativePos);
	do
	{
		int NextGroupIndex = GroupListIndex + MoveDir;
		tl_swap(m_Map.m_aGroupIDList[GroupListIndex], m_Map.m_aGroupIDList[NextGroupIndex]);
		GroupListIndex = NextGroupIndex;
	} while(GroupListIndex != NewGroupListIndex);

	char aHistoryEntryAction[64];
	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Group %d: change order"), OldGroupIndex);
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "%d > %d", OldGroupIndex, NewGroupListIndex);
	HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);

	return NewGroupListIndex;
}

int CEditor2::EditLayerOrderMove(int ParentGroupListIndex, int LayerListIndex, int RelativePos)
{
	// Returns new parent group list index (or the same if it does not change)
	if(RelativePos == 0)
		return ParentGroupListIndex;

	dbg_assert(ParentGroupListIndex >= 0 && ParentGroupListIndex < m_Map.m_GroupIDListCount, "ParentGroupListIndex out of bounds");
	dbg_assert(m_Map.m_aGroups.IsValid(m_Map.m_aGroupIDList[ParentGroupListIndex]), "ParentGroup is invalid");

	const int OldParentGroupListIndex = ParentGroupListIndex;
	const int OldLayerListIndex = LayerListIndex;

	// let's move one at a time
	const int RelativeDir = clamp(RelativePos, -1, 1);
	for(; RelativePos != 0; RelativePos -= RelativeDir)
	{
		CEditorMap2::CGroup& ParentGroup = m_Map.m_aGroups.Get(m_Map.m_aGroupIDList[ParentGroupListIndex]);
		dbg_assert(LayerListIndex >= 0 && LayerListIndex < ParentGroup.m_LayerCount, "LayerID out of bounds");

		const u32 LayerID = ParentGroup.m_apLayerIDs[LayerListIndex];
		const int NewPos = LayerListIndex + RelativeDir;
		const bool IsGameLayer = LayerID == (u32)m_Map.m_GameLayerID;
		const bool CurrentGroupIsOpen = m_UiGroupState[m_Map.m_aGroupIDList[ParentGroupListIndex]].m_IsOpen;

		// go up one group
		if(NewPos < 0 || (!CurrentGroupIsOpen && RelativeDir == -1))
		{
			if(ParentGroupListIndex > 0 && !IsGameLayer)
			{
				CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(m_Map.m_aGroupIDList[ParentGroupListIndex-1]);

				// TODO: make a function GroupAddLayer?
				if(Group.m_LayerCount < CEditorMap2::MAX_GROUP_LAYERS)
					Group.m_apLayerIDs[Group.m_LayerCount++] = LayerID;

				if(NewPos < 0)
				{
					// squash layer from previous group
					ParentGroup.m_LayerCount--;
					memmove(&ParentGroup.m_apLayerIDs[0], &ParentGroup.m_apLayerIDs[1], sizeof(ParentGroup.m_apLayerIDs[0]) * ParentGroup.m_LayerCount);
				}
				else
				{
					dbg_assert(LayerListIndex == ParentGroup.m_LayerCount-1, "shouldn't happen, we are on an intermediary hidden group, going up");
					// "remove" layer from previous group
					ParentGroup.m_LayerCount--;
				}			

				ParentGroupListIndex--;
				LayerListIndex = Group.m_LayerCount-1;
			}
		}
		// go down one group
		else if(NewPos >= ParentGroup.m_LayerCount || (!CurrentGroupIsOpen && RelativeDir == +1))
		{
			if(ParentGroupListIndex < m_Map.m_GroupIDListCount-1 && !IsGameLayer)
			{
				CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(m_Map.m_aGroupIDList[ParentGroupListIndex+1]);

				// move other layers down, put this one first
				if(Group.m_LayerCount < CEditorMap2::MAX_GROUP_LAYERS)
				{
					memmove(&Group.m_apLayerIDs[1], &Group.m_apLayerIDs[0], sizeof(Group.m_apLayerIDs[0]) * Group.m_LayerCount);
					Group.m_apLayerIDs[0] = LayerID;
					Group.m_LayerCount++;
				}

				if(NewPos >= ParentGroup.m_LayerCount)
				{
					// "remove" layer from previous group
					ParentGroup.m_LayerCount--;
				}
				else
				{
					dbg_assert(LayerListIndex == 0, "shouldn't happen, we are on an intermediary hidden group, going down");
					/// squash layer from previous group
					ParentGroup.m_LayerCount--;
					memmove(&ParentGroup.m_apLayerIDs[0], &ParentGroup.m_apLayerIDs[1], sizeof(ParentGroup.m_apLayerIDs[0]) * ParentGroup.m_LayerCount);				
				}
				
				ParentGroupListIndex++;
				LayerListIndex = 0;
			}
		}
		else
		{
			tl_swap(ParentGroup.m_apLayerIDs[LayerListIndex], ParentGroup.m_apLayerIDs[NewPos]);
			LayerListIndex = NewPos;
		}
	}

	char aHistoryEntryAction[64];
	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Layer %d: change order"), OldLayerListIndex);
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "Group:(%d > %d)  Layer:(%d > %d)", OldParentGroupListIndex, ParentGroupListIndex, OldLayerListIndex, LayerListIndex);
	HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);

	return ParentGroupListIndex;
}

void CEditor2::EditTileSelectionFlipX(int LayerID)
{
	dbg_assert(m_Map.m_aLayers.IsValid(LayerID), "LayerID out of bounds");

	TileLayerRegionToBrush(LayerID, m_TileSelection.m_StartTX, m_TileSelection.m_StartTY,
		m_TileSelection.m_EndTX, m_TileSelection.m_EndTY);
	BrushFlipX();
	BrushPaintLayer(m_TileSelection.m_StartTX, m_TileSelection.m_StartTY, LayerID);
	BrushClear();

	char aHistoryEntryAction[64];
	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Layer %d: tile selection"),
		LayerID);
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "Flipped X");
	HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
}

void CEditor2::EditTileSelectionFlipY(int LayerID)
{
	dbg_assert(m_Map.m_aLayers.IsValid(LayerID), "LayerID out of bounds");

	TileLayerRegionToBrush(LayerID, m_TileSelection.m_StartTX, m_TileSelection.m_StartTY,
		m_TileSelection.m_EndTX, m_TileSelection.m_EndTY);
	BrushFlipY();
	BrushPaintLayer(m_TileSelection.m_StartTX, m_TileSelection.m_StartTY, LayerID);
	BrushClear();

	char aHistoryEntryAction[64];
	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Layer %d: tile selection"),
		LayerID);
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "Flipped Y");
	HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
}

void CEditor2::EditBrushPaintLayer(int PaintTX, int PaintTY, int LayerID)
{
	dbg_assert(m_Map.m_aLayers.IsValid(LayerID), "LayerID out of bounds");
	dbg_assert(m_Map.m_aLayers.Get(LayerID).IsTileLayer(), "Layer is not a tile layer");

	BrushPaintLayer(PaintTX, PaintTY, LayerID);

	char aHistoryEntryAction[64];
	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Layer %d: brush paint"),
		LayerID);
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "at (%d, %d)",
		PaintTX, PaintTY);
	HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
}

void CEditor2::EditBrushPaintLayerFillRectRepeat(int PaintTX, int PaintTY, int PaintW, int PaintH, int LayerID)
{
	dbg_assert(m_Map.m_aLayers.IsValid(LayerID), "LayerID out of bounds");
	dbg_assert(m_Map.m_aLayers.Get(LayerID).IsTileLayer(), "Layer is not a tile layer");

	BrushPaintLayerFillRectRepeat(PaintTX, PaintTY, PaintW, PaintH, LayerID);

	char aHistoryEntryAction[64];
	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Layer %d: brush paint"),
		LayerID);
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "at (%d, %d)(%d, %d)",
		PaintTX, PaintTY, PaintW, PaintH);
	HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
}

void CEditor2::EditBrushPaintLayerAutomap(int PaintTX, int PaintTY, int LayerID, int RulesetID)
{
	dbg_assert(m_Map.m_aLayers.IsValid(LayerID), "LayerID out of bounds");
	dbg_assert(m_Map.m_aLayers.Get(LayerID).IsTileLayer(), "Layer is not a tile layer");

	BrushPaintLayer(PaintTX, PaintTY, LayerID);

	CEditorMap2::CLayer& Layer = m_Map.m_aLayers.Get(LayerID);
	dbg_assert(Layer.m_ImageID >= 0 && Layer.m_ImageID < m_Map.m_Assets.m_ImageCount, "ImageID out of bounds or invalid");

	CTilesetMapper2* pMapper = m_Map.AssetsFindTilesetMapper(Layer.m_ImageID);

	dbg_assert(pMapper != 0, "Tileset mapper not found");
	dbg_assert(Layer.m_aTiles.size() == Layer.m_Width*Layer.m_Height, "Tile count does not match layer size");

	pMapper->AutomapLayerSection(Layer.m_aTiles.base_ptr(), PaintTX, PaintTY, m_Brush.m_Width, m_Brush.m_Height, Layer.m_Width, Layer.m_Height, RulesetID);

	char aHistoryEntryAction[64];
	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Layer %d: brush paint auto"),
		LayerID);
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "at (%d, %d)",
		PaintTX, PaintTY);
	HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
}

void CEditor2::EditBrushPaintLayerFillRectAutomap(int PaintTX, int PaintTY, int PaintW, int PaintH, int LayerID, int RulesetID)
{
	dbg_assert(m_Map.m_aLayers.IsValid(LayerID), "LayerID out of bounds");
	dbg_assert(m_Map.m_aLayers.Get(LayerID).IsTileLayer(), "Layer is not a tile layer");

	BrushPaintLayerFillRectRepeat(PaintTX, PaintTY, PaintW, PaintH, LayerID);

	CEditorMap2::CLayer& Layer = m_Map.m_aLayers.Get(LayerID);
	dbg_assert(Layer.m_ImageID >= 0 && Layer.m_ImageID < m_Map.m_Assets.m_ImageCount, "ImageID out of bounds or invalid");

	CTilesetMapper2* pMapper = m_Map.AssetsFindTilesetMapper(Layer.m_ImageID);

	dbg_assert(pMapper != 0, "Tileset mapper not found");
	dbg_assert(Layer.m_aTiles.size() == Layer.m_Width*Layer.m_Height, "Tile count does not match layer size");

	pMapper->AutomapLayerSection(Layer.m_aTiles.base_ptr(), PaintTX, PaintTY, PaintW, PaintH, Layer.m_Width, Layer.m_Height, RulesetID);

	char aHistoryEntryAction[64];
	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Layer %d: brush paint auto"),
		LayerID);
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "at (%d, %d)(%d, %d)",
		PaintTX, PaintTY, PaintW, PaintH);
	HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
}

void CEditor2::EditTileLayerResize(int LayerID, int NewWidth, int NewHeight)
{
	dbg_assert(m_Map.m_aLayers.IsValid(LayerID), "LayerID out of bounds");
	dbg_assert(m_Map.m_aLayers.Get(LayerID).IsTileLayer(), "Layer is not a tile layer");
	dbg_assert(NewWidth > 0 && NewHeight > 0, "NewWidth, NewHeight invalid");

	CEditorMap2::CLayer& Layer = m_Map.m_aLayers.Get(LayerID);

	if(NewWidth == Layer.m_Width && NewHeight == Layer.m_Height)
		return;

	const int OldWidth = Layer.m_Width;
	const int OldHeight = Layer.m_Height;
	array2<CTile>& aTiles = Layer.m_aTiles;

	CTile* pCopyBuff = (CTile*)mem_alloc(OldWidth * OldHeight * sizeof(CTile), 1); // TODO: use ring buffer?

	dbg_assert(aTiles.size() == OldWidth * OldHeight, "Tile count does not match Width*Height");
	memmove(pCopyBuff, aTiles.base_ptr(), sizeof(CTile) * OldWidth * OldHeight);

	aTiles.clear();
	aTiles.add_empty(NewWidth * NewHeight);

	for(int oty = 0; oty < OldHeight; oty++)
	{
		for(int otx = 0; otx < OldWidth; otx++)
		{
			int oid = oty * OldWidth + otx;
			if(oty > NewHeight-1 || otx > NewWidth-1)
				continue;
			aTiles[oty * NewWidth + otx] = pCopyBuff[oid];
		}
	}

	mem_free(pCopyBuff);

	Layer.m_Width = NewWidth;
	Layer.m_Height = NewHeight;

	char aHistoryEntryAction[64];
	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Layer %d: resized"),
		LayerID);
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "(%d, %d) > (%d, %d)",
		OldWidth, OldHeight,
		NewWidth, NewHeight);
	HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
}

void CEditor2::EditTileLayerAutoMapWhole(int LayerID, int RulesetID)
{
	dbg_assert(m_Map.m_aLayers.IsValid(LayerID), "LayerID out of bounds");

	CEditorMap2::CLayer& Layer = m_Map.m_aLayers.Get(LayerID);

	dbg_assert(Layer.m_ImageID >= 0 && Layer.m_ImageID < m_Map.m_Assets.m_ImageCount, "ImageID out of bounds or invalid");

	CTilesetMapper2* pMapper = m_Map.AssetsFindTilesetMapper(Layer.m_ImageID);

	dbg_assert(pMapper != 0, "Tileset mapper not found");
	dbg_assert(Layer.m_aTiles.size() == Layer.m_Width*Layer.m_Height, "Tile count does not match layer size");

	pMapper->AutomapLayerWhole(Layer.m_aTiles.base_ptr(), Layer.m_Width, Layer.m_Height, RulesetID);

	char aHistoryEntryAction[64];
	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Layer %d: tileset automap"),
		LayerID);
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "Ruleset: %s", pMapper->GetRuleSetName(RulesetID));
	HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
}

void CEditor2::EditTileLayerAutoMapSection(int LayerID, int RulesetID, int StartTx, int StartTy, int SectionWidth, int SectionHeight)
{
	dbg_assert(m_Map.m_aLayers.IsValid(LayerID), "LayerID out of bounds");

	CEditorMap2::CLayer& Layer = m_Map.m_aLayers.Get(LayerID);

	dbg_assert(Layer.m_ImageID >= 0 && Layer.m_ImageID < m_Map.m_Assets.m_ImageCount, "ImageID out of bounds or invalid");

	CTilesetMapper2* pMapper = m_Map.AssetsFindTilesetMapper(Layer.m_ImageID);

	dbg_assert(pMapper != 0, "Tileset mapper not found");
	dbg_assert(Layer.m_aTiles.size() == Layer.m_Width*Layer.m_Height, "Tile count does not match layer size");

	pMapper->AutomapLayerSection(Layer.m_aTiles.base_ptr(), StartTx, StartTy, SectionWidth, SectionHeight, Layer.m_Width, Layer.m_Height, RulesetID);

	char aHistoryEntryAction[64];
	char aHistoryEntryDesc[64];
	str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Layer %d: tileset automap section"),
		LayerID);
	str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "Ruleset: %s", pMapper->GetRuleSetName(RulesetID));
	HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
}

void CEditor2::EditHistCondLayerChangeName(int LayerID, const char* pNewName, bool HistoryCondition)
{
	dbg_assert(m_Map.m_aLayers.IsValid(LayerID), "LayerID out of bounds");

	CEditorMap2::CLayer& Layer = m_Map.m_aLayers.Get(LayerID);

	// names are indentical, stop
	if(str_comp(Layer.m_aName, pNewName) == 0)
		return;

	char aOldName[sizeof(Layer.m_aName)];
	str_copy(aOldName, Layer.m_aName, sizeof(aOldName));
	str_copy(Layer.m_aName, pNewName, sizeof(Layer.m_aName));

	if(HistoryCondition)
	{
		char aHistoryEntryAction[64];
		char aHistoryEntryDesc[64];
		str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Layer %d: changed name"),
			LayerID);
		str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "'%s' -> '%s'", aOldName, pNewName);
		HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
	}
}

void CEditor2::EditHistCondLayerChangeColor(int LayerID, vec4 NewColor, bool HistoryCondition)
{
	dbg_assert(m_Map.m_aLayers.IsValid(LayerID), "LayerID out of bounds");

	CEditorMap2::CLayer& Layer = m_Map.m_aLayers.Get(LayerID);

	// colors are indentical, stop
	if(mem_comp(&Layer.m_Color, &NewColor, sizeof(NewColor)) == 0)
		return;

	Layer.m_Color = NewColor;

	if(HistoryCondition)
	{
		char aHistoryEntryAction[64];
		char aHistoryEntryDesc[64];
		str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("%s: changed layer color"),
			GetLayerName(LayerID));
		str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc),
			"(%.2f, %.2f, %.2f, %.2f)", NewColor.r, NewColor.g, NewColor.b, NewColor.a);
		HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
	}
}

void CEditor2::EditHistCondGroupChangeName(int GroupID, const char* pNewName, bool HistoryCondition)
{
	dbg_assert(m_Map.m_aGroups.IsValid(GroupID), "GroupID out of bounds");

	CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(GroupID);

	// names are indentical, stop
	if(str_comp(Group.m_aName, pNewName) == 0)
		return;

	char aOldName[sizeof(Group.m_aName)];
	str_copy(aOldName, Group.m_aName, sizeof(aOldName));
	str_copy(Group.m_aName, pNewName, sizeof(Group.m_aName));

	if(HistoryCondition)
	{
		char aHistoryEntryAction[64];
		char aHistoryEntryDesc[64];
		str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Group %d: changed name"),
			GroupID);
		str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "'%s' -> '%s'", aOldName, pNewName);
		HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
	}
}

void CEditor2::EditHistCondGroupChangeParallax(int GroupID, int NewParallaxX, int NewParallaxY, bool HistoryCondition)
{
	dbg_assert(m_Map.m_aGroups.IsValid(GroupID), "GroupID out of bounds");

	CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(GroupID);
	if(NewParallaxX == Group.m_ParallaxX && NewParallaxY == Group.m_ParallaxY)
		return;

	// sanitize user input
	NewParallaxX = clamp(NewParallaxX, CEditorMap2::Limits::GroupParallaxMin, CEditorMap2::Limits::GroupParallaxMax);
	NewParallaxY = clamp(NewParallaxY, CEditorMap2::Limits::GroupParallaxMin, CEditorMap2::Limits::GroupParallaxMax);

	const int OldParallaxX = Group.m_ParallaxX;
	const int OldParallaxY = Group.m_ParallaxY;
	Group.m_ParallaxX = NewParallaxX;
	Group.m_ParallaxY = NewParallaxY;

	if(HistoryCondition)
	{
		char aHistoryEntryAction[64];
		char aHistoryEntryDesc[64];
		str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Group %d: changed parallax"),
			GroupID);
		str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "(%d, %d) > (%d, %d)",
			OldParallaxX, OldParallaxY,
			NewParallaxX, NewParallaxY);
		HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
	}
}

void CEditor2::EditHistCondGroupChangeOffset(int GroupID, int NewOffsetX, int NewOffsetY, bool HistoryCondition)
{
	dbg_assert(m_Map.m_aGroups.IsValid(GroupID), "GroupID out of bounds");

	CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(GroupID);
	if(NewOffsetX == Group.m_OffsetX && NewOffsetY == Group.m_OffsetY)
		return;

	// sanitize user input
	NewOffsetX = clamp(NewOffsetX, CEditorMap2::Limits::GroupOffsetMin, CEditorMap2::Limits::GroupOffsetMax);
	NewOffsetY = clamp(NewOffsetY, CEditorMap2::Limits::GroupOffsetMin, CEditorMap2::Limits::GroupOffsetMax);

	const int OldOffsetX = Group.m_OffsetX;
	const int OldOffsetY = Group.m_OffsetY;
	Group.m_OffsetX = NewOffsetX;
	Group.m_OffsetY = NewOffsetY;

	if(HistoryCondition)
	{
		char aHistoryEntryAction[64];
		char aHistoryEntryDesc[64];
		str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Group %d: changed offset"),
			GroupID);
		str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "(%d, %d) > (%d, %d)",
			OldOffsetX, OldOffsetY,
			NewOffsetX, NewOffsetY);
		HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
	}
}

void CEditor2::EditHistCondGroupChangeClipX(int GroupID, int NewClipX, bool HistoryCondition)
{
	dbg_assert(m_Map.m_aGroups.IsValid(GroupID), "GroupID out of bounds");

	CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(GroupID);

	if(NewClipX == Group.m_ClipX)
		return;

	int OldClipX = Group.m_ClipX;
	int OldClipWidth = Group.m_ClipWidth;
	Group.m_ClipWidth += Group.m_ClipX - NewClipX;
	Group.m_ClipX = NewClipX;
	if(Group.m_ClipWidth < 0)
		Group.m_ClipWidth = 0;

	if(HistoryCondition)
	{
		char aHistoryEntryAction[64];
		char aHistoryEntryDesc[64];
		str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Group %d: changed clip left"), GroupID);
		str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "(%d, %d) > (%d, %d)",
			OldClipX, OldClipWidth,
			Group.m_ClipY, Group.m_ClipHeight);
		HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
	}
}

void CEditor2::EditHistCondGroupChangeClipY(int GroupID, int NewClipY, bool HistoryCondition)
{
	dbg_assert(m_Map.m_aGroups.IsValid(GroupID), "GroupID out of bounds");

	CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(GroupID);

	if(NewClipY == Group.m_ClipY)
		return;

	int OldClipY = Group.m_ClipY;
	int OldClipHeight = Group.m_ClipHeight;
	Group.m_ClipHeight += Group.m_ClipY - NewClipY;
	Group.m_ClipY = NewClipY;
	if(Group.m_ClipHeight < 0)
		Group.m_ClipHeight = 0;

	if(HistoryCondition)
	{
		char aHistoryEntryAction[64];
		char aHistoryEntryDesc[64];
		str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Group %d: changed clip top"),
			GroupID);
		str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "(%d, %d) > (%d, %d)",
			OldClipY, OldClipHeight,
			Group.m_ClipY, Group.m_ClipHeight);
		HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
	}
}

void CEditor2::EditHistCondGroupChangeClipRight(int GroupID, int NewClipRight, bool HistoryCondition)
{
	dbg_assert(m_Map.m_aGroups.IsValid(GroupID), "GroupID out of bounds");

	CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(GroupID);

	if(NewClipRight - Group.m_ClipX == Group.m_ClipWidth)
		return;

	int OldClipWidth = Group.m_ClipWidth;
	Group.m_ClipWidth = NewClipRight - Group.m_ClipX;
	if(Group.m_ClipWidth < 0)
		Group.m_ClipWidth = 0;

	if(HistoryCondition)
	{
		char aHistoryEntryAction[64];
		char aHistoryEntryDesc[64];
		str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Group %d: changed clip width"), GroupID);
		str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "%d > %d",
			OldClipWidth, Group.m_ClipWidth);
		HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
	}
}

void CEditor2::EditHistCondGroupChangeClipBottom(int GroupID, int NewClipBottom, bool HistoryCondition)
{
	dbg_assert(m_Map.m_aGroups.IsValid(GroupID), "GroupID out of bounds");

	CEditorMap2::CGroup& Group = m_Map.m_aGroups.Get(GroupID);

	if(NewClipBottom - Group.m_ClipY == Group.m_ClipHeight)
		return;

	int OldClipHeight = Group.m_ClipHeight;
	Group.m_ClipHeight = NewClipBottom - Group.m_ClipY;
	if(Group.m_ClipHeight < 0)
		Group.m_ClipHeight = 0;

	if(HistoryCondition)
	{
		char aHistoryEntryAction[64];
		char aHistoryEntryDesc[64];
		str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Group %d: changed clip height"), GroupID);
		str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "%d > %d",
			OldClipHeight, Group.m_ClipHeight);
		HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
	}
}

void CEditor2::EditHistCondBrushPaintStrokeOnLayer(int StartTX, int StartTY, int EndTX, int EndTY, int LayerID, bool HistoryCondition)
{
	dbg_assert(m_Map.m_aLayers.IsValid(LayerID), "LayerID out of bounds");
	dbg_assert(m_Map.m_aLayers.Get(LayerID).IsTileLayer(), "Layer is not a tile layer");

	// TODO: trace from Start to End
	BrushPaintLayer(EndTX, EndTY, LayerID);

	if(HistoryCondition)
	{
		char aHistoryEntryAction[64];
		char aHistoryEntryDesc[64];
		str_format(aHistoryEntryAction, sizeof(aHistoryEntryAction), Localize("Layer %d: brush paint"), LayerID);
		str_format(aHistoryEntryDesc, sizeof(aHistoryEntryDesc), "(%d, %d)", EndTX, EndTY);
		HistoryNewEntry(aHistoryEntryAction, aHistoryEntryDesc);
	}
}

void CEditor2::ConLoad(IConsole::IResult* pResult, void* pUserData)
{
	CEditor2 *pSelf = (CEditor2 *)pUserData;

	const int InputTextLen = str_length(pResult->GetString(0));

	char aMapPath[256];
	bool AddMapPath = str_comp_nocase_num(pResult->GetString(0), "maps/", 5) != 0;
	bool AddMapExtension = InputTextLen <= 4 ||
		str_comp_nocase_num(pResult->GetString(0)+InputTextLen-4, ".map", 4) != 0;
	str_format(aMapPath, sizeof(aMapPath), "%s%s%s", AddMapPath ? "maps/":"", pResult->GetString(0),
			   AddMapExtension ? ".map":"");

	dbg_msg("editor", "ConLoad(%s)", aMapPath);
	pSelf->LoadMap(aMapPath);
}

void CEditor2::ConSave(IConsole::IResult* pResult, void* pUserData)
{
	CEditor2 *pSelf = (CEditor2 *)pUserData;

	const int InputTextLen = str_length(pResult->GetString(0));

	char aMapPath[256];
	bool AddMapPath = str_comp_nocase_num(pResult->GetString(0), "maps/", 5) != 0;
	bool AddMapExtension = InputTextLen <= 4 ||
		str_comp_nocase_num(pResult->GetString(0)+InputTextLen-4, ".map", 4) != 0;
	str_format(aMapPath, sizeof(aMapPath), "%s%s%s", AddMapPath ? "maps/":"", pResult->GetString(0),
			   AddMapExtension ? ".map":"");

	dbg_msg("editor", "ConSave(%s)", aMapPath);
	pSelf->SaveMap(aMapPath);
}

void CEditor2::ConShowPalette(IConsole::IResult* pResult, void* pUserData)
{
	// CEditor2 *pSelf = (CEditor2 *)pUserData;
	dbg_assert(0, "Implement this");
}

void CEditor2::ConGameView(IConsole::IResult* pResult, void* pUserData)
{
	CEditor2 *pSelf = (CEditor2 *)pUserData;
	pSelf->m_ConfigShowGameEntities = (pResult->GetInteger(0) > 0);
	pSelf->m_ConfigShowExtendedTilemaps = pSelf->m_ConfigShowGameEntities;
}

void CEditor2::ConShowGrid(IConsole::IResult* pResult, void* pUserData)
{
	CEditor2 *pSelf = (CEditor2 *)pUserData;
	pSelf->m_ConfigShowGrid = (pResult->GetInteger(0) > 0);
}

void CEditor2::ConUndo(IConsole::IResult* pResult, void* pUserData)
{
	CEditor2 *pSelf = (CEditor2 *)pUserData;
	pSelf->HistoryUndo();
}

void CEditor2::ConRedo(IConsole::IResult* pResult, void* pUserData)
{
	CEditor2 *pSelf = (CEditor2 *)pUserData;
	pSelf->HistoryRedo();
}

void CEditor2::ConDeleteImage(IConsole::IResult* pResult, void* pUserData)
{
	CEditor2 *pSelf = (CEditor2 *)pUserData;
	pSelf->EditDeleteImage(pResult->GetInteger(0));
}
