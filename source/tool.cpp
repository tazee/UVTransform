//
// UVTransform - UVTransform - Transform UV values on UV and 3D spaces. 
//

#include "tool.hpp"
#include "util.hpp"

#include <iostream>
#include <format>

/*
 * On create we add our one tool attribute. We also allocate a vector type
 * and select mode mask.
 */
CUVTransform::CUVTransform()
{
    CLxUser_PacketService sPkt;
    CLxUser_MeshService   sMesh;

    dyna_Add(ATTRs_TRANS_U, LXsTYPE_UVCOORD);
    dyna_Add(ATTRs_TRANS_V, LXsTYPE_UVCOORD);
    dyna_Add(ATTRs_ANGLE, LXsTYPE_ANGLE);
    dyna_Add(ATTRs_SCALE_U, LXsTYPE_PERCENT);
    dyna_Add(ATTRs_SCALE_V, LXsTYPE_PERCENT);
    dyna_Add(ATTRs_CENTER_U, LXsTYPE_UVCOORD);
    dyna_Add(ATTRs_CENTER_V, LXsTYPE_UVCOORD);
    dyna_Add(ATTRs_TWEAK, LXsTYPE_BOOLEAN);

    tool_Reset();

    sPkt.NewVectorType(LXsCATEGORY_TOOL, v_type);
    sPkt.AddPacket(v_type, LXsP_TOOL_VIEW_EVENT, LXfVT_GET);
    sPkt.AddPacket(v_type, LXsP_TOOL_SCREEN_EVENT, LXfVT_GET);
    sPkt.AddPacket(v_type, LXsP_TOOL_FALLOFF, LXfVT_GET);
    sPkt.AddPacket(v_type, LXsP_TOOL_SUBJECT2, LXfVT_GET);
    sPkt.AddPacket(v_type, LXsP_TOOL_INPUT_EVENT, LXfVT_GET);
	sPkt.AddPacket (v_type, LXsP_TOOL_EVENTTRANS,  LXfVT_GET);
	sPkt.AddPacket (v_type, LXsP_TOOL_ACTCENTER,   LXfVT_GET);

    offset_view = sPkt.GetOffset(LXsCATEGORY_TOOL, LXsP_TOOL_VIEW_EVENT);
    offset_screen = sPkt.GetOffset(LXsCATEGORY_TOOL, LXsP_TOOL_SCREEN_EVENT);
    offset_falloff = sPkt.GetOffset(LXsCATEGORY_TOOL, LXsP_TOOL_FALLOFF);
    offset_subject = sPkt.GetOffset(LXsCATEGORY_TOOL, LXsP_TOOL_SUBJECT2);
    offset_input = sPkt.GetOffset(LXsCATEGORY_TOOL, LXsP_TOOL_INPUT_EVENT);
	offset_event  = sPkt.GetOffset (LXsCATEGORY_TOOL, LXsP_TOOL_EVENTTRANS);
	offset_center = sPkt.GetOffset (LXsCATEGORY_TOOL, LXsP_TOOL_ACTCENTER);
	offset_xfrm   = sPkt.GetOffset (LXsCATEGORY_TOOL, LXsP_TOOL_XFRM);
	offset_raycast = sPkt.GetOffset(LXsCATEGORY_TOOL, LXsP_TOOL_RAYCAST);
    mode_select = sMesh.SetMode("select");

    LXx_VUNIT(m_axis, 2);
    LXx_VCLR(m_trans0);
    LXx_VCLR(m_center0);
}

/*
 * Reset sets the attributes back to defaults.
 */
void CUVTransform::tool_Reset()
{
    dyna_Value(ATTRa_TRANS_U).SetFlt(0.0);
    dyna_Value(ATTRa_TRANS_V).SetFlt(0.0);
    dyna_Value(ATTRa_ANGLE).SetFlt(0.0);
    dyna_Value(ATTRa_SCALE_U).SetFlt(1.0);
    dyna_Value(ATTRa_SCALE_V).SetFlt(1.0);
    dyna_Value(ATTRa_CENTER_U).SetFlt(0.5);
    dyna_Value(ATTRa_CENTER_V).SetFlt(0.5);
    dyna_Value(ATTRa_TWEAK).SetInt(0);
}

/*
 * Boilerplate methods that identify this as an action (state altering) tool.
 */
LXtObjectID CUVTransform::tool_VectorType()
{
    return v_type.m_loc;  // peek method; does not add-ref
}

const char* CUVTransform::tool_Order()
{
    return LXs_ORD_ACTR;
}

LXtID4 CUVTransform::tool_Task()
{
    return LXi_TASK_ACTR;
}

/*
 * We employ the simplest possible tool model -- default hauling. We indicate
 * that we want to haul one attribute, we name the attribute, and we implement
 * Initialize() which is what to do when the tool activates or re-activates.
 * In this case set the axis to the current value.
 */
unsigned CUVTransform::tmod_Flags()
{
    return LXfTMOD_I0_INPUT | LXfTMOD_DRAW_3D | LXfTMOD_AUTORESET | LXfTMOD_ROLLOVERS;
}

LxResult CUVTransform::tmod_Enable(ILxUnknownID obj)
{
    CLxUser_Message msg(obj);
    unsigned int primary_index = 0;

    if (TestVertex(primary_index) == false)
    {
        msg.SetCode(LXe_CMD_DISABLED);
        msg.SetMessage(SRVNAME_TOOL, "NoVertex", 0);
        return LXe_DISABLED;
    }
    return LXe_OK;
}

bool CUVTransform::UVMapSetup(CLxUser_MeshMap& vmap)
{
    LxResult result;

    CLxUser_VMapPacketTranslation vmap_pkt_trans;
    vmap_pkt_trans.autoInit ();

    LXtID4	vmapSelType	  = s_sel.LookupType (LXsSELTYP_VERTEXMAP);
    unsigned vmapSelCount = s_sel.Count (vmapSelType);
    for (auto i = 0; i < vmapSelCount; i++) {
        void* pkt = s_sel.ByIndex (vmapSelType, i);
        LXtID4 type;
        vmap_pkt_trans.Type (pkt, &type);
        if (type == LXi_VMAP_TEXTUREUV) {
            const char* name;
            vmap_pkt_trans.Name (pkt, &name);
            result = vmap.SelectByName(type, name);
            if (result == LXe_OK)
                return true;
        }
    }
    return false;
}

LxResult CUVTransform::HitPolygon(CLxUser_VectorStack& vec)
{
    LXpToolScreenEvent*  spak = static_cast<LXpToolScreenEvent*>(vec.Read(offset_screen));
    CLxUser_RaycastPacket rayPkt;
	vec.ReadObject(offset_raycast, rayPkt);

	LXtHitElement hit;
    if (rayPkt.HitClosest(vec, LXf_LAYER_ACTIVE, spak->fcx, spak->fcy, &hit))
    {
        std::string id4 = lx::ID4String(hit.type);
        printf("[HIT] id4 %s poly %p wPos %f %f %f isUV (%d)\n", id4.c_str(), hit.pol, hit.wPos[0], hit.wPos[1], hit.wPos[2], hit.isUV);
	    CLxUser_Mesh mesh;
        CLxUser_Item item;

        mesh.set(hit.mesh);
        item.set(hit.item);
        LXtMatrix4 xfrm;
        CLxUser_Locator locator(item);
        CLxUser_Scene scene(item);
        CLxUser_ChannelRead chan(scene);
		scene.GetChannels (chan, 0.0);
        locator.WorldTransform4(chan, xfrm); 
        m_space.m_vmap.fromMesh(mesh);
        if (!UVMapSetup(m_space.m_vmap))
            return LXe_FALSE;
        m_space.SetPolygon(mesh, xfrm, hit);
        SetTweakElement(hit);
        return LXe_TRUE;
    }
    return LXe_FALSE;
}

LxResult CUVTransform::SetTweakElement(LXtHitElement& hit)
{
    m_polygon.clear();
    m_edge.clear();
    m_point.clear();

    CLxUser_Mesh mesh;
    CLxUser_Item item;

    mesh.set(hit.mesh);
    item.set(hit.item);

    if (hit.type == LXiSEL_VERTEX)
    {
        m_point.fromMesh(mesh);
        m_point.Select(hit.vrt);
    }
    else if (hit.type == LXiSEL_EDGE)
    {
        m_edge.fromMesh(mesh);
        m_edge.Select(hit.edge);
    }
    else if (hit.type == LXiSEL_POLYGON)
    {
        m_polygon.fromMesh(mesh);
        m_polygon.Select(hit.pol);
    }
    else if (hit.type == lx::StringID4("DISC"))
    {
        m_point.fromMesh(mesh);
        m_polygon.fromMesh(mesh);
        m_point.Select(hit.vrt);
        m_polygon.Select(hit.pol);
    }
    else if (hit.type == lx::StringID4("DSED"))
    {
        m_edge.fromMesh(mesh);
        m_polygon.fromMesh(mesh);
        m_edge.Select(hit.edge);
        m_polygon.Select(hit.pol);
    }
    else
        return LXe_FALSE;
    return LXe_TRUE;
}

CLxMatrix4 CUVTransform::GetRotateMatrix()
{
    int x, y;
    auto viewId = s_v3d.Mouse (&x, &y);
    CLxUser_View3D	view3D;
    s_v3d.View (viewId, view3D);
    if (view3D.Space() == lx::StringID4("UV2D"))
        return CLxMatrix4();
    LXtVector ax, ay, az, pos;
    view3D.To3D(x, y, pos, 0);
    view3D.To3D(x+1, y, ax, 0);
    view3D.To3D(x, y+1, ay, 0);
    LXx_VSUB(ax, pos);
    LXx_VSUB(ay, pos);
    lx::VectorNormalize(ax);
    lx::VectorNormalize(ay);
    LXx_VNEG(ay);
    LXx_VCROSS(az, ax, ay);
/*
    printf("screen ax %f %f %f\n", ax[0], ax[1], ax[2]);
    printf("       ay %f %f %f\n", ay[0], ay[1], ay[2]);
    printf("       az %f %f %f\n", az[0], az[1], az[2]);
    printf(" x = %d y = %d space (%s)\n", x, y, lx::ID4String(view3D.Space()).c_str());
*/
    LXtMatrix m;
    LXx_VSET3(m[0], ax[0], ay[0], az[0]);
    LXx_VSET3(m[1], ax[1], ay[1], az[1]);
    LXx_VSET3(m[2], ax[2], ay[2], az[2]);
    return CLxMatrix4(m);
}

LxResult CUVTransform::tmod_Down(ILxUnknownID vts, ILxUnknownID adjust)
{
	CLxUser_AdjustTool	 at (adjust);
	CLxUser_VectorStack	 vec (vts);
	LXpToolActionCenter* acen = static_cast<LXpToolActionCenter*>(vec.Read (offset_center));
	LXpToolInputEvent*   ipak = static_cast<LXpToolInputEvent*>(vec.Read(offset_input));
    LXpToolScreenEvent*  spak = static_cast<LXpToolScreenEvent*>(vec.Read(offset_screen));
	LXpToolViewEvent*    view = static_cast<LXpToolViewEvent *>(vec.Read (offset_view));

    CLxUser_EventTranslatePacket epkt;
	vec.ReadObject (offset_event, epkt);
    epkt.GetNewPosition(vts, m_mousePos);

    printf("[DOWN] pos %f %f %f part (%x)\n", m_mousePos[0], m_mousePos[1], m_mousePos[2], ipak->part);
    m_part = ipak->part;

    LXtVector dir, pos;
    double dx, dy;

    int x, y;
    auto viewId = s_v3d.Mouse (&x, &y);
    m_view_matrix = GetRotateMatrix();
    m_view_matrix_inv = m_view_matrix.inverse();

    switch (m_part)
    {
        case HANDLE_TRANS_U:
            dyna_Value(ATTRa_TRANS_U).GetFlt(&m_trans0[0]);
            dyna_Value(ATTRa_CENTER_U).GetFlt(&m_center0[0]);
            LXx_VSET3(dir, m_view_matrix[0][0], m_view_matrix[0][1], m_view_matrix[0][2]);
            epkt.SetLinearConstraint(vts, m_mousePos, dir);
            break;
        case HANDLE_TRANS_V:
            dyna_Value(ATTRa_TRANS_V).GetFlt(&m_trans0[1]);
            dyna_Value(ATTRa_CENTER_V).GetFlt(&m_center0[1]);
            LXx_VSET3(dir, m_view_matrix[1][0], m_view_matrix[1][1], m_view_matrix[1][2]);
            epkt.SetLinearConstraint(vts, m_mousePos, dir);
            break;
        case HANDLE_SCALE_U:
            dyna_Value(ATTRa_SCALE_U).GetFlt(&m_scale0[0]);
            LXx_VSET3(dir, m_view_matrix[0][0], m_view_matrix[0][1], m_view_matrix[0][2]);
            epkt.SetLinearConstraint(vts, m_mousePos, dir);
            break;
        case HANDLE_SCALE_V:
            dyna_Value(ATTRa_SCALE_V).GetFlt(&m_scale0[1]);
            LXx_VSET3(dir, m_view_matrix[1][0], m_view_matrix[1][1], m_view_matrix[1][2]);
            epkt.SetLinearConstraint(vts, m_mousePos, dir);
            break;
        case HANDLE_PLANE:
            dyna_Value(ATTRa_TRANS_U).GetFlt(&m_trans0[0]);
            dyna_Value(ATTRa_TRANS_V).GetFlt(&m_trans0[1]);
            dyna_Value(ATTRa_CENTER_U).GetFlt(&m_center0[0]);
            dyna_Value(ATTRa_CENTER_V).GetFlt(&m_center0[1]);
            if (view->type == LXi_VIEWTYPE_3D)
            {
                LXx_VSET3(dir, m_view_matrix[2][0], m_view_matrix[2][1], m_view_matrix[2][2]);
                CLxVector pos3D = m_space.PosUVto3D(m_center0[0], m_center0[1]);
                epkt.HitHandle(vts, pos3D.v);
                epkt.SetPlanarConstraint(vts, pos3D.v, dir);
            }
            break;
        case HANDLE_CENTER:
            dyna_Value(ATTRa_TRANS_U).GetFlt(&m_trans0[0]);
            dyna_Value(ATTRa_TRANS_V).GetFlt(&m_trans0[1]);
            dyna_Value(ATTRa_CENTER_U).GetFlt(&m_center0[0]);
            dyna_Value(ATTRa_CENTER_V).GetFlt(&m_center0[1]);
            if (view->type == LXi_VIEWTYPE_3D)
            {
                CLxVector norm = m_space.TriangleNormal();
                CLxVector pos3D = m_space.PosUVto3D(m_center0[0], m_center0[1]);
                epkt.HitHandle(vts, pos3D.v);
                epkt.SetPlanarConstraint(vts, pos3D.v, norm.v);
            }
            break;
        case HANDLE_ROTATE:
            dyna_Value(ATTRa_CENTER_U).GetFlt(&m_center0[0]);
            dyna_Value(ATTRa_CENTER_V).GetFlt(&m_center0[1]);
            dyna_Value(ATTRa_ANGLE).GetFlt(&m_angle0);
            epkt.ToModel(vts, m_mousePos, m_axis);
            if (view->type == LXi_VIEWTYPE_3D)
            {
                CLxVector pos3D = m_space.PosUVto3D(m_center0[0], m_center0[1]);
                LXx_VCPY(pos, pos3D.v);
            }
            else
            {
                LXx_VCPY(pos, m_center0);
            }
            m_rotHandle.MouseDown(viewId, pos, m_axis, m_mousePos);
            epkt.HitHandle(vts, m_mousePos);
            break;
        default:
            if (HitPolygon(vec) == LXe_TRUE)
            {
                at.SetFlt(ATTRa_CENTER_U, m_space.m_centerUV.v[0]);
                at.SetFlt(ATTRa_CENTER_V, m_space.m_centerUV.v[1]);
            }
            dyna_Value(ATTRa_TRANS_U).GetFlt(&m_trans0[0]);
            dyna_Value(ATTRa_TRANS_V).GetFlt(&m_trans0[1]);
            dyna_Value(ATTRa_CENTER_U).GetFlt(&m_center0[0]);
            dyna_Value(ATTRa_CENTER_V).GetFlt(&m_center0[1]);
            if (view->type == LXi_VIEWTYPE_3D)
            {
                LXx_VSET3(dir, m_view_matrix[2][0], m_view_matrix[2][1], m_view_matrix[2][2]);
                epkt.SetPlanarConstraint(vts, m_mousePos, dir);
            }
            break;
    }

    at.Invalidate();
    return LXe_TRUE;
}

void CUVTransform::tmod_Move(ILxUnknownID vts, ILxUnknownID adjust)
{
    CLxUser_AdjustTool at(adjust);
    CLxUser_VectorStack vec(vts);
    LXpToolScreenEvent*  spak = static_cast<LXpToolScreenEvent*>(vec.Read(offset_screen));
	LXpToolInputEvent*   ipak = static_cast<LXpToolInputEvent*>(vec.Read(offset_input));
	LXpToolViewEvent*    view = static_cast<LXpToolViewEvent *>(vec.Read (offset_view));

    CLxUser_RaycastPacket rayPkt;
	vec.ReadObject(offset_raycast, rayPkt);

    CLxUser_EventTranslatePacket epkt;
	vec.ReadObject (offset_event, epkt);
    LXtVector new_pos, axis;
    epkt.GetNewPosition(vts, new_pos);
    //epkt.ToModel(vts, pos, axis);

    LXtMatrix xfrm;
    CLxVector pos0 = CLxVector(m_mousePos);
    CLxVector pos1 = CLxVector(new_pos);
    if (view->type == LXi_VIEWTYPE_3D)
    {
        pos0 *= m_view_matrix_inv;
        pos1 *= m_view_matrix_inv;
    }
    double trans_u = pos1[0] - pos0[0];
    double trans_v = pos1[1] - pos0[1];
    printf("[MOVE] part (%d) new_pos %f %f %f scale %f\n", m_part, new_pos[0], new_pos[1], new_pos[2], m_space.m_3d_uv_scale);
    if (view->type == LXi_VIEWTYPE_3D)
    {
        trans_u *= m_space.m_3d_uv_scale;
        trans_v *= m_space.m_3d_uv_scale;
    }
    switch (m_part)
    {
        case HANDLE_TRANS_U:
            at.SetFlt(ATTRa_TRANS_U, m_trans0[0] + trans_u);
            break;
        case HANDLE_TRANS_V:
            at.SetFlt(ATTRa_TRANS_V, m_trans0[1] + trans_v);
            break;
        case HANDLE_PLANE:
            at.SetFlt(ATTRa_TRANS_U, m_trans0[0] + trans_u);
            at.SetFlt(ATTRa_TRANS_V, m_trans0[1] + trans_v);
            break;
        case HANDLE_SCALE_U:
            m_offset = trans_u;
            at.SetFlt(ATTRa_SCALE_U, m_scale0[0] + m_offset);
            if (ipak->mode & IQ_CONSTRAIN)
                at.SetFlt(ATTRa_SCALE_V, m_scale0[0] + m_offset);
            break;
        case HANDLE_SCALE_V:
            m_offset = trans_v;
            at.SetFlt(ATTRa_SCALE_V, m_scale0[1] + m_offset);
            if (ipak->mode & IQ_CONSTRAIN)
                at.SetFlt(ATTRa_SCALE_U, m_scale0[1] + m_offset);
            break;
        case HANDLE_CENTER:
            if (view->type == LXi_VIEWTYPE_3D)
            {
                CLxVector uv = m_space.Pos3DtoUV(new_pos);
                at.SetFlt(ATTRa_CENTER_U, uv[0]);
                at.SetFlt(ATTRa_CENTER_V, uv[1]);
            }
            else
            {
                at.SetFlt(ATTRa_CENTER_U, m_center0[0] + trans_u);
                at.SetFlt(ATTRa_CENTER_V, m_center0[1] + trans_v);
            }
            break;
        case HANDLE_ROTATE:
            m_view_matrix.getMatrix3x3(xfrm);
            m_rotHandle.MouseMove(new_pos);
            m_rotHandle.GetAngles(2, xfrm, &m_sAngle, &m_eAngle);
            at.SetFlt(ATTRa_ANGLE, m_angle0 + m_rotHandle.m_angle);
            break;
        default:
            at.SetFlt(ATTRa_TRANS_U, trans_u);
            at.SetFlt(ATTRa_TRANS_V, trans_v);
            break;
    }
    at.Invalidate();
}

void CUVTransform::tmod_Up(ILxUnknownID vts, ILxUnknownID adjust)
{
    CLxUser_AdjustTool at(adjust);
    CLxUser_VectorStack vec(vts);
    m_part = -1;
    m_offset = 0.0;
    m_sAngle = 0.0;
    m_eAngle = 0.0;
    at.Invalidate();
}

void CUVTransform::DrawHandles (ILxUnknownID vts, ILxUnknownID stroke, int flags)
{
	CLxUser_VectorStack	 vec (vts);
	CLxUser_StrokeDraw	 draw (stroke);
	CLxUser_HandleDraw	 handle (stroke);
	LXtVector		 pos, color, view_pos;
	LXtMatrix		 m, view_matrix;
	int			 dFlags = 0;

	LXpToolViewEvent	*view = (LXpToolViewEvent *) vec.Read (offset_view);
	if (!view)
		return;

    lx::MatrixIdent(m);
    LXx_VCLR(pos);

    double centerX, centerY;
    dyna_Value(ATTRa_CENTER_U).GetFlt(&centerX);
    dyna_Value(ATTRa_CENTER_V).GetFlt(&centerY);

    if (view->type == LXi_VIEWTYPE_3D)
    {
        CLxUser_View view(stroke);
        LXtVector ax, ay, az;
        lx::MatrixIdent(view_matrix);
        CLxVector center3D = m_space.PosUVto3D(centerX, centerY);
        LXx_VCPY(view_pos, center3D.v);
        view.ScreenNormals(view_pos, ax, ay, az);
        LXx_VNEG(ay);
        LXx_VCROSS(az, ax, ay);
        LXx_VSET3(view_matrix[0], ax[0], ay[0], az[0]);
        LXx_VSET3(view_matrix[1], ax[1], ay[1], az[1]);
        LXx_VSET3(view_matrix[2], ax[2], ay[2], az[2]);
    }
    else
    {
        lx::MatrixIdent(view_matrix);
        LXx_VCLR(view_pos);
        LXx_VSET3(view_pos, centerX, centerY, 0.0);
    }
	draw.PushTransform (view_pos, view_matrix);

    // Center Handle
	dFlags = (m_part == HANDLE_CENTER) ? LXi_THANDf_HOT : 0;
	handle.Handle (pos, 0, HANDLE_CENTER, dFlags);

    // Translate Handles
	dFlags = (m_part == HANDLE_TRANS_U) ? LXi_THANDf_HOT : 0;
	handle.MoveHandle (pos, m, 0, HANDLE_TRANS_U, dFlags);

	dFlags = (m_part == HANDLE_TRANS_V) ? LXi_THANDf_HOT : 0;
	handle.MoveHandle (pos, m, 1, HANDLE_TRANS_V, dFlags);

    // Scale Handles
	dFlags = (m_part == HANDLE_SCALE_U) ? LXi_THANDf_HOT : 0;
	ScaleHandle (stroke, pos, m, 0, HANDLE_SCALE_U, m_offset, dFlags, view->type);

	dFlags = (m_part == HANDLE_SCALE_V) ? LXi_THANDf_HOT : 0;
	ScaleHandle (stroke, pos, m, 1, HANDLE_SCALE_V, m_offset, dFlags, view->type);

    // Rotate Handle
    //m_sAngle = 0.0;
    //m_eAngle = 45.0 * LXx_DEG2RAD;
	dFlags = (m_part == HANDLE_ROTATE) ? LXi_THANDf_HOT : 0;
	handle.RotateHandle (pos, m, 2, HANDLE_ROTATE, m_sAngle, m_eAngle, 0, dFlags);
	//handle.RotateMouseHandle (pos, m_mousePos, m, 2, HANDLE_ROTATE, dFlags);

    // Plane Handle
	dFlags = (m_part == HANDLE_PLANE) ? LXi_THANDf_HOT : 0;
	handle.PlaneHandle (pos, m, 2, HANDLE_PLANE, dFlags);

	draw.PopTransform ();
}

void CUVTransform::ScaleHandle(ILxUnknownID stroke, const LXtVector pos, const LXtMatrix m, int axis, int part, double offset, int flags, int type)
{
	CLxUser_StrokeDraw	 draw (stroke);
	CLxUser_ShapeDraw	 shape (stroke);
	CLxUser_HandleDraw	 handle (stroke);
    const LXtVector axisColor[] = {
        {0.9, 0.2, 0.2}, {0.2, 0.8, 0.2}, {0.2, 0.4, 1.0}
    };
    const LXtVector axisColorUV[] = {
        {0.708, 0.644, 0.160}, {0.345, 0.648, 0.750}, {0.2, 0.4, 1.0}
    };

    double length = 6.0, alpha= 0.95;
    length += length * 0.1;
    LXtVector col;
    if (flags & LXi_THANDf_HOT)
        LXx_VSET3(col, 0.8, 0.6, 1.0);
    else if (type == LXi_VIEWTYPE_UV)
        LXx_VCPY(col, axisColorUV[axis]);
    else
        LXx_VCPY(col, axisColor[axis]);
    LXtVector boxPos, boxSize;
    LXx_VSET(boxSize, 0.25);
    LXx_VCLR(boxPos);
    boxPos[axis] = length - boxSize[0] + offset;

    draw.PushTransform(pos, m);

    draw.SetPart(part);
    shape.CubeFill(col, alpha, boxPos, boxSize, LXiSTROKE_SCREEN);

    if (offset != 0.0)
    {
        boxPos[axis] = length - boxSize[0];
        shape.CubeFill(col, 0.3, boxPos, boxSize, LXiSTROKE_SCREEN);
    }

    draw.PopTransform();
}

void CUVTransform::tmod_Draw (ILxUnknownID vts, ILxUnknownID stroke, int flags)
{
	CLxUser_VectorStack	 vec (vts);
	CLxUser_StrokeDraw	 draw (stroke);
	LXpToolViewEvent	*view = (LXpToolViewEvent *) vec.Read (offset_view);

    if (view->type == LXi_VIEWTYPE_3D)
        m_space.DrawPolygon3D(draw, view);
    else
        m_space.DrawPolygonUV(draw, view);

    DrawHandles(vts, stroke, flags);
}

void CUVTransform::tmod_Test (ILxUnknownID vts, ILxUnknownID stroke, int flags)
{
    DrawHandles(vts, stroke, flags);
}

void CUVTransform::tmod_Initialize (ILxUnknownID vts, ILxUnknownID adjust, unsigned int flags)
{
    CLxUser_AdjustTool at(adjust);
    at.SetFlt(ATTRa_TRANS_U, 0.0);
    at.SetFlt(ATTRa_TRANS_V, 0.0);
    at.SetFlt(ATTRa_ANGLE, 0.0);
    at.SetFlt(ATTRa_SCALE_U, 1.0);
    at.SetFlt(ATTRa_SCALE_V, 1.0);
}

LxResult CUVTransform::atrui_DisableMsg (unsigned int index, ILxUnknownID msg)
{
    return LXe_OK;
}

LxResult CUVTransform::atrui_UIHints(unsigned int index, ILxUnknownID hints)
{
	CLxLoc_UIHints		 uiHints(hints);

    if ((index == ATTRa_SCALE_U) || (index == ATTRa_SCALE_V))
    {
        uiHints.MinFloat(0.0);
    }
    return LXe_OK;
}

bool CUVTransform::TestVertex(unsigned int& primary_index)
{
    /*
     * Start the scan in read-only mode.
     */
    CLxUser_LayerScan scan;
    CLxUser_Mesh      mesh;
    unsigned          i, n, count;
    bool              ok = false;

    primary_index = 0;

    s_layer.BeginScan(LXf_LAYERSCAN_ACTIVE | LXf_LAYERSCAN_MARKVERTS, scan);

    if (scan)
    {
        n = scan.NumLayers();
        for (i = 0; i < n; i++)
        {
            scan.BaseMeshByIndex(i, mesh);
            mesh.PointCount(&count);
            if (count > 0)
            {
                ok = true;
                primary_index = i;
                break;
            }
        }
        scan.Apply();
    }

    /*
     * Return false if there is no polygons in any active layers.
     */
    return ok;
}

class VertexVisitor : public CLxImpl_AbstractVisitor
{
public:
    LxResult Evaluate()
    {
        m_vert.SetMarks(m_mark_done);
        return LXe_OK;
    }
    CLxUser_Mesh    m_mesh;
    CLxUser_Point   m_vert;
    LXtMarkMode     m_mark_done;
};

class PolygonVisitor : public CLxImpl_AbstractVisitor
{
public:
    LxResult Evaluate()
    {
        if (m_tweak && (m_tweak_polyID != nullptr))
        {
            if (m_poly.ID() != m_tweak_polyID)
                return LXe_OK;
        }
        unsigned int nvert;
        int index;
        m_poly.VertexCount(&nvert);
        m_poly.Index(&index);
        //printf("polygon (%d) nvert %u tweak (%d)\n", index, nvert, m_tweak);
        for (auto i = 0u; i < nvert; i++)
        {
            LXtPointID vertID;
            m_poly.VertexByIndex(i, &vertID);
            m_vert.Select(vertID);
            unsigned index;
            m_vert.Index(&index);
            if (m_vert.TestMarks(m_mark_pick) == LXe_FALSE)
                continue;
            if (m_tweak && (m_tweak_vertID != nullptr))
            {
                if (m_vert.ID() != m_tweak_vertID)
                    continue;
            }
            if (m_tweak && (m_tweak_edgeID != nullptr))
            {
                LXtPointID v0, v1;
                m_edge.Select(m_tweak_edgeID);
                m_edge.Endpoints(&v0, &v1);
                if ((vertID != v0) && (vertID != v1))
                    continue;
            }
            float value[2], new_value[2];
            if (m_poly.MapValue(m_vmap.ID(), vertID, value) == LXe_OK)
            {
                if (m_vert.TestMarks(m_mark_pick) == LXe_TRUE)
                {
                    double u = (value[0] - m_center[0]) * m_scale[0];
                    double v = (value[1] - m_center[1]) * m_scale[1];
                    double x = u * m_cost - v * m_sint + m_center[0] + m_trans[0];
                    double y = v * m_cost + u * m_sint + m_center[1] + m_trans[1];
                    new_value[0] = static_cast<float>(x);
                    new_value[1] = static_cast<float>(y);
                    m_poly.SetMapValue(vertID, m_vmap.ID(), new_value);
                    //printf("[%u] disco vert %u\n", i, index);
                    TransformConnected(value, new_value);
                }
            }
            else if (m_vert.TestMarks(m_mark_done) == LXe_FALSE)
            {
                if (m_vert.MapValue(m_vmap.ID(), value) == LXe_OK)
                {
                    double u = (value[0] - m_center[0]) * m_scale[0];
                    double v = (value[1] - m_center[1]) * m_scale[1];
                    double x = u * m_cost - v * m_sint + m_center[0] + m_trans[0];
                    double y = v * m_cost + u * m_sint + m_center[1] + m_trans[1];
                    new_value[0] = static_cast<float>(x);
                    new_value[1] = static_cast<float>(y);
                    m_vert.SetMapValue(m_vmap.ID(), new_value);
                    //printf("[%u] cont vert %u\n", i, index);
                    TransformConnected(value, new_value);
                }
                m_vert.SetMarks(m_mark_done);
            }
        }
        return LXe_OK;
    }

    void TransformConnected(float* base, float* new_value)
    {
        unsigned npoly;
        m_vert.PolygonCount(&npoly);
        CLxUser_Polygon upoly;
        upoly.fromMesh(m_mesh);
        for (auto j = 0u; j < npoly; j++)
        {
            LXtPolygonID polyID;
            m_vert.PolygonByIndex(j, &polyID);
            if (polyID == m_poly.ID())
                continue;
            upoly.Select(polyID);
            if (m_tweak == false)
            {
                if (upoly.TestMarks(m_mark_pick) == LXe_TRUE)
                    continue;
            }
            float value1[2];
            int index;
            upoly.Index(&index);
            if (upoly.MapEvaluate(m_vmap.ID(), m_vert.ID(), value1) == LXe_OK)
            {
                if ((lx::Compare(value1[0], base[0]) == LXi_EQUAL_TO)
                    &&(lx::Compare(value1[1], base[1]) == LXi_EQUAL_TO))
                {
                    upoly.SetMapValue(m_vert.ID(), m_vmap.ID(), new_value);
                    //printf("[%u] connected poly %d\n", j, index);
                }
            }
        }
    }

    CLxUser_Mesh    m_mesh;
    CLxUser_Polygon m_poly;
    CLxUser_Point   m_vert;
    CLxUser_Edge    m_edge;
    CLxUser_MeshMap m_vmap;
    LXtMarkMode     m_mark_pick;
    LXtMarkMode     m_mark_done;
    double          m_trans[2], m_scale[2], m_angle, m_center[2];
    double          m_sint, m_cost;
    bool            m_tweak;
    bool            m_tearOff;
    LXtPolygonID    m_tweak_polyID;
    LXtEdgeID       m_tweak_edgeID;
    LXtPointID      m_tweak_vertID;
};

bool CUVTransform::GetCurrentUVMap(std::string& uvName)
{
    CLxUser_SelectionService sel_svc;
    CLxUser_VMapPacketTranslation	 vmap_pkt_trans;
	LXtID4 vmapSelType = sel_svc.LookupType (LXsSELTYP_VERTEXMAP);
    LXtID4 selVmapType;
    const char* name = nullptr;
    unsigned vmapSelCount = sel_svc.Count (vmapSelType);
    vmap_pkt_trans.autoInit();
    for (unsigned i = 0; i < vmapSelCount; i++) {
            void* pkt = sel_svc.ByIndex (vmapSelType, i);
            vmap_pkt_trans.Type (pkt, &selVmapType);
            if (selVmapType == LXi_VMAP_TEXTUREUV) {
                vmap_pkt_trans.Name (pkt, &name);
                uvName = name;
                return true;
            }
    }
    return false;
}

/*
 * Tool evaluation uses layer scan interface to walk through all the active
 * meshes and visit all the selected polygons.
 */
void CUVTransform::tool_Evaluate(ILxUnknownID vts)
{
    std::cout << "CUVTransform::tool_Evaluate: " << std::endl;

    CLxUser_VectorStack vec(vts);
    CLxUser_Subject2Packet subject;
    if (vec.ReadObject(offset_subject, subject) == false)
        return;

	LXpToolViewEvent* viewEvent = (LXpToolViewEvent *) vec.Read (offset_view);
    if (!viewEvent)
        return;

    std::string name;
    if (GetCurrentUVMap(name) == false)
        return;

    PolygonVisitor vis;
    int tweak;
    dyna_Value(ATTRa_TRANS_U).GetFlt(&vis.m_trans[0]);
    dyna_Value(ATTRa_TRANS_V).GetFlt(&vis.m_trans[1]);
    dyna_Value(ATTRa_ANGLE).GetFlt(&vis.m_angle);
    dyna_Value(ATTRa_SCALE_U).GetFlt(&vis.m_scale[0]);
    dyna_Value(ATTRa_SCALE_V).GetFlt(&vis.m_scale[1]);
    dyna_Value(ATTRa_CENTER_U).GetFlt(&vis.m_center[0]);
    dyna_Value(ATTRa_CENTER_V).GetFlt(&vis.m_center[1]);
    dyna_Value(ATTRa_TWEAK).GetInt(&tweak);
    vis.m_cost = std::cos(vis.m_angle);
    vis.m_sint = std::sin(vis.m_angle);
    vis.m_center[0] -= vis.m_trans[0];
    vis.m_center[1] -= vis.m_trans[1];
    vis.m_tweak = tweak;
    vis.m_tearOff = false;
    vis.m_tweak_polyID = m_polygon.test() ? m_polygon.ID() : nullptr;
    vis.m_tweak_edgeID = m_edge.test() ? m_edge.ID() : nullptr;
    vis.m_tweak_vertID = m_point.test() ? m_point.ID() : nullptr;

    if (vis.m_tweak)
    {
        if (!m_polygon.test() && !m_edge.test() && !m_point.test())
            return;
    }

    CLxUser_LayerScan  scan;
    subject.BeginScan(LXf_LAYERSCAN_EDIT_POLVRT, scan);
    auto count = scan.NumLayers();

    for (auto i = 0u; i < count; i++)
    {
        CLxUser_Mesh mesh, edit;
        scan.BaseMeshByIndex(i, mesh);
        VertexVisitor vvis;
        vvis.m_vert.fromMesh(mesh);
        vvis.m_mark_done = mesh_svc.ClearMode(LXsMARK_USER_0);
        vvis.m_vert.Enum(&vvis, LXiMARK_ANY);
        scan.EditMeshByIndex(i, edit);
        vis.m_mesh = edit;
        vis.m_vmap.fromMesh(edit);
        vis.m_vmap.SelectByName(LXi_VMAP_TEXTUREUV, name.c_str());
        vis.m_poly.fromMesh(edit);
        vis.m_vert.fromMesh(edit);
        vis.m_edge.fromMesh(edit);
        vis.m_mark_done = mesh_svc.SetMode(LXsMARK_USER_0);
        vis.m_mark_pick = mesh_svc.SetMode(LXsMARK_SELECT);
        if (subject.Type() == LXiSEL_POLYGON)
            vis.m_poly.Enum(&vis, vis.m_mark_pick);
        else
            vis.m_poly.Enum(&vis, LXiMARK_ANY);
        //scan.SetMeshChange(i, LXf_MESHEDIT_MAP_UV);
        scan.SetMeshChange(i, LXf_MESHEDIT_GEOMETRY);
    }

    scan.Apply();
}

/*
 * Export tool server.
 */
void initialize()
{
    CLxGenericPolymorph* srv;

    srv = new CLxPolymorph<CUVTransform>;
    srv->AddInterface(new CLxIfc_Tool<CUVTransform>);
    srv->AddInterface(new CLxIfc_ToolModel<CUVTransform>);
    srv->AddInterface(new CLxIfc_Attributes<CUVTransform>);
    srv->AddInterface(new CLxIfc_AttributesUI<CUVTransform>);
    srv->AddInterface(new CLxIfc_ChannelUI<CUVTransform>);
    lx::AddServer(SRVNAME_TOOL, srv);
}
