//
// UVTransform - Transform UV values on UV and 3D spaces. 
//

#pragma once

#include <lxsdk/lxu_attributes.hpp>
#include <lxsdk/lxu_select.hpp>
#include <lxsdk/lxu_attributes.hpp>
#include <lxsdk/lxu_vector.hpp>
#include <lxsdk/lxu_math.hpp>
#include <lxsdk/lxu_format.hpp>

#include <lxsdk/lx_layer.hpp>
#include <lxsdk/lx_mesh.hpp>
#include <lxsdk/lx_log.hpp>
#include <lxsdk/lx_plugin.hpp>
#include <lxsdk/lx_seltypes.hpp>
#include <lxsdk/lx_tool.hpp>
#include <lxsdk/lx_toolui.hpp>
#include <lxsdk/lx_layer.hpp>
#include <lxsdk/lx_vector.hpp>
#include <lxsdk/lx_pmodel.hpp>
#include <lxsdk/lx_vmodel.hpp>
#include <lxsdk/lx_channelui.hpp>
#include <lxsdk/lx_draw.hpp>
#include <lxsdk/lx_handles.hpp>
#include <lxsdk/lx_vp.hpp>

#include <lxsdk/lx_value.hpp>
#include <lxsdk/lx_select.hpp>
#include <lxsdk/lx_seltypes.hpp>
#include <lxsdk/lx_locator.hpp>
#include <lxsdk/lxvalue.h>

#include "util.hpp"
#include "space.hpp"

using namespace lx_err;

const char* SRVNAME_TOOL = "tool.uvTransform";

#define ATTRs_TRANS_U   "transU"
#define ATTRs_TRANS_V   "transV"
#define ATTRs_ANGLE     "angle"
#define ATTRs_SCALE_U   "scaleU"
#define ATTRs_SCALE_V   "scaleV"
#define ATTRs_CENTER_U  "centerU"
#define ATTRs_CENTER_V  "centerV"
#define ATTRs_TWEAK     "tweak"

#define ATTRa_TRANS_U   0
#define ATTRa_TRANS_V   1
#define ATTRa_ANGLE     2
#define ATTRa_SCALE_U   3
#define ATTRa_SCALE_V   4
#define ATTRa_CENTER_U  5
#define ATTRa_CENTER_V  6
#define ATTRa_TWEAK     7

#define HANDLE_CENTER   0x1000
#define HANDLE_TRANS_U  0x2000
#define HANDLE_TRANS_V  0x2001
#define HANDLE_SCALE_U  0x3000
#define HANDLE_SCALE_V  0x3001
#define HANDLE_ROTATE   0x4000
#define HANDLE_PLANE    0x5000

#define IQ_CONSTRAIN		0x08

#ifndef LXx_OVERRIDE
#define LXx_OVERRIDE override
#endif

/*
 * Basic tool and tool model methods are defined here. The
 * attributes interface is inherited from the utility class.
 */

class CUVTransform : public CLxImpl_Tool, public CLxImpl_ToolModel, public CLxDynamicAttributes, public CLxImpl_ChannelUI
{
public:
    CUVTransform();

    void        tool_Reset() LXx_OVERRIDE;
    LXtObjectID tool_VectorType() LXx_OVERRIDE;
    const char* tool_Order() LXx_OVERRIDE;
    LXtID4      tool_Task() LXx_OVERRIDE;
	void		tool_Evaluate   (ILxUnknownID vts) LXx_OVERRIDE;

    unsigned    tmod_Flags() LXx_OVERRIDE;
	void        tmod_Initialize (ILxUnknownID vts, ILxUnknownID adjust, unsigned int flags) LXx_OVERRIDE;
    LxResult    tmod_Enable(ILxUnknownID obj) LXx_OVERRIDE;
    LxResult    tmod_Down(ILxUnknownID vts, ILxUnknownID adjust) LXx_OVERRIDE;
    void        tmod_Move(ILxUnknownID vts, ILxUnknownID adjust) LXx_OVERRIDE;
    void        tmod_Up(ILxUnknownID vts, ILxUnknownID adjust) LXx_OVERRIDE;
	void        tmod_Draw (ILxUnknownID vts, ILxUnknownID stroke, int flags) LXx_OVERRIDE;
	void        tmod_Test (ILxUnknownID vts, ILxUnknownID stroke, int flags) LXx_OVERRIDE;

    LxResult	atrui_DisableMsg (unsigned int index, ILxUnknownID msg) LXx_OVERRIDE;
	LxResult	atrui_UIHints   (unsigned int index, ILxUnknownID hints) LXx_OVERRIDE;

    bool TestVertex(unsigned int& primary_index);
    bool GetCurrentUVMap(std::string& uvName);
	void DrawHandles (ILxUnknownID vts, ILxUnknownID stroke, int flags);
    void ScaleHandle(ILxUnknownID stroke, const LXtVector pos, const LXtMatrix m, int axis, int part, double offset, int flags, int type);
    LxResult HitPolygon(CLxUser_VectorStack& vec);
    bool UVMapSetup(CLxUser_MeshMap& vmap);
    LxResult HitTweakPolygon(CLxUser_VectorStack& vec);

    CLxUser_LogService   s_log;
    CLxUser_LayerService s_layer;
    CLxUser_VectorType   v_type;
    CLxUser_SelectionService s_sel;
    CLxUser_MeshService mesh_svc;
    CLxUser_View3DportService s_v3d;
	CLxUser_ValueService	 s_val;

    unsigned offset_view;
    unsigned offset_screen;
    unsigned offset_falloff;
    unsigned offset_subject;
    unsigned offset_input;
    unsigned offset_event;
	unsigned offset_center;
	unsigned offset_xfrm;
    unsigned offset_raycast;
    unsigned mode_select;
	
	LXtItemType m_itemType;
	CLxUser_Value val_poly;

    double  m_angle0;
    double  m_sAngle, m_eAngle, m_offset;
    LXtVector m_trans0, m_scale0, m_center0;
    LXtVector m_mousePos, m_axis;
    LXtMatrix m_xfrm;
    CRotationHandle m_rotHandle;
    CSpaceTransform m_space;
    int m_part;

    // Tweak mode
    int m_tweak;
    CLxUser_Point m_point;
    CLxUser_Polygon m_polygon;
    CLxUser_Edge m_edge;
};

