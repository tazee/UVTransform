//
// AutoPartByDistance - A plugin to group polygon parts that are close together. 
//

#include "space.hpp"
#include "lxsdk/lxdraw.h"
#include "util.hpp"

bool CSpaceTransform::SetPolygon(CLxUser_Mesh& mesh, const LXtMatrix4 matrix, const LXtHitElement* hit)
{
    m_polygon.clear();
    m_mesh = mesh;
    m_matrix = CLxMatrix4(matrix);

    lx::Matrix4Ident(m_3d_uv_matrix);
    m_point.fromMesh(mesh);
    m_polygon.fromMesh(mesh);
    m_edge.fromMesh(mesh);

    LXtPolygonID polyID = nullptr;
    if (hit->type == LXiSEL_VERTEX)
    {
        m_point.Select(hit->vrt);
        m_point.PolygonByIndex(0, &polyID);
    }
    else if (hit->type == LXiSEL_EDGE)
    {
        m_edge.Select(hit->edge);
        m_edge.PolygonByIndex(0, &polyID);
    }
    else if (hit->type == LXiSEL_POLYGON)
        polyID = hit->pol;

    if (!polyID)
        return false;

    m_item.set(hit->item);
    m_scene.from(m_item);

    m_polygon.Select(polyID);
    unsigned int nvert;
    m_polygon.VertexCount(&nvert);
    printf("SetPolygon: vrt=%p edge=%p pol=%p, nvert=%u\n", hit->vrt, hit->edge, hit->pol, nvert);
    if (nvert < 3)
    {
        m_polygon.clear();
        return false;
    }
    m_box3D.clear();
    m_boxUV.clear();
    positions3D.resize(nvert);
    positionsUV.resize(nvert);
    for (unsigned int i = 0; i < nvert; i++)
    {
        LXtPointID vrtID;
        m_polygon.VertexByIndex(i, &vrtID);
        LXtFVector pos3d, posUV, posW;
        m_point.Select(vrtID);
        m_point.Pos(pos3d);
        lx::Matrix4Multiply(posW, matrix, pos3d);
        m_box3D.add(posW);
        positions3D[i] = CLxVector(posW);
        LXx_VCLR(posUV);
        m_polygon.MapEvaluate(m_vmap.ID(), vrtID, posUV);
        m_boxUV.add(posUV);
        positionsUV[i] = CLxVector(posUV);
        printf("[%u] posUV %f %f %f\n", i, posUV[0], posUV[1], posUV[2]);
    }
    LXtVector vec;
    m_polygon.Normal(vec);
    CLxVector normal(vec);

    CLxVector v1 = positions3D[1] - positions3D[0];
    CLxVector v2 = positions3D.back() - positions3D[0];
    CLxMatrix4 p_basis(
        v1[0], v2[0], normal[0], 0.0,
        v1[1], v2[1], normal[1], 0.0,
        v1[2], v2[2], normal[2], 0.0,
        0.0, 0.0, 0.0, 1.0
    );

    CLxVector u1(positionsUV[1] - positionsUV[0]);
    CLxVector u2(positionsUV.back() - positionsUV[0]);
    CLxMatrix4 u_basis(
        u1[0], u2[0], 0.0, 0.0,
        u1[1], u2[1], 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    );

    CLxVector orig3D = positions3D[0];
    CLxVector origUV = positionsUV[0];
    CLxMatrix4 trans = p_basis * u_basis.inverse();
    CLxVector offset = origUV - trans * orig3D;
    m_3d_uv_matrix = CLxMatrix4(trans, offset);

    CLxVector v(positions3D[0]);
    v1.normalize();
    v2.normalize();
    CLxVector v3 = v1.cross(v2);
    v2 = v3.cross(v1);
    int iz = MathUtil::MaxExtent(v3.v);
    LXx_VUNIT(v1.v, (iz + 1) % 3);
    if (v3.v[iz] < 0.0)
    {
        LXx_VSCL(v3.v, -1.0);
        printf("*** NEGATIVE Z-AXIS ***\n");
    }
    v2 = v3.cross(v1);
    v1 = v2.cross(v3);
    int ix = MathUtil::MaxExtent(v1.v);
    if (ix == 1)
    {
        std::swap(v1, v2);
        //v3 = v1.cross(v2);
        printf("*** SWAP AXES ***\n");
    }
    LXtMatrix m;
    LXx_VSET3(m[0], v1[0], v2[0], v3[0]);
    LXx_VSET3(m[1], v1[1], v2[1], v3[1]);
    LXx_VSET3(m[2], v1[2], v2[2], v3[2]);
    m_view_matrix = CLxMatrix4(m);
    m_view_matrix.setTranslation(v);
    m_view_matrix_inv = m_view_matrix.inverse();

    CLxVector plane0 = MathUtil::ProjectPointToUV(positions3D[0], positions3D[1], positions3D.back(), positions3D[0]);
    CLxVector plane1 = MathUtil::ProjectPointToUV(positions3D[0], positions3D[1], positions3D.back(), positions3D[1]);
    CLxVector plane2 = MathUtil::ProjectPointToUV(positions3D[0], positions3D[1], positions3D.back(), positions3D.back());
    CLxVector p1(plane1 - plane0);
    CLxVector p2(plane2 - plane0);

    m_3d_uv_scale = p1.length() / u1.length();
    m_uv_3d_rotate = MathUtil::MatrixVectorRotation(u1, p1);
    printf("plane0     %f %f %f\n",plane0[0], plane0[1], plane0[2]);
    printf("plane1     %f %f %f\n",plane1[0], plane1[1], plane1[2]);
    printf("plane2     %f %f %f\n",plane2[0], plane2[1], plane2[2]);
    printf("dot        %f\n", p1[0] * u1[0] + p1[1] * u1[1]);
    printf("rot_matrix %f %f %f\n", m_uv_3d_rotate[0][0], m_uv_3d_rotate[0][1], m_uv_3d_rotate[0][2]);
    printf("           %f %f %f\n", m_uv_3d_rotate[1][0], m_uv_3d_rotate[1][1], m_uv_3d_rotate[1][2]);
    printf("           %f %f %f\n", m_uv_3d_rotate[2][0], m_uv_3d_rotate[2][1], m_uv_3d_rotate[2][2]);

    printf("rot angle = %f\n", MathUtil::AngleVectors(u1, p1) * LXx_RAD2DEG);
    printf("v1 %f %f %f\n", v1[0], v1[1], v1[2]);
    printf("v2 %f %f %f\n", v2[0], v2[1], v2[2]);
    printf("v3 %f %f %f\n", v3[0], v3[1], v3[2]);

    return true;
}


void CSpaceTransform::DrawPolygon3D(CLxUser_StrokeDraw& draw, const LXpToolViewEvent* view)
{
    if (m_polygon.test() == false)
        return;
    
    if (view == nullptr)
        return;

    int index;
    m_polygon.Index(&index);
    //printf("DrawPolygon: (%d) npol=%u, type=%u\n", index, m_poly_info.npol, m_poly_info.type);
    LXtVector p0, draw_rgb;
    LXx_VSET3(draw_rgb, 1.0, 0.4, 0.4);

    LXtMatrix m;
    LXtVector v;

    if (view->type == LXi_VIEWTYPE_3D)
    {
        m_matrix.getMatrix3x3(m);
        CLxVector translation = m_matrix.getTranslation();
        LXx_VCPY(v, translation.v);
    }
    else
    {
        return;
    }

    m_polygon.Tessellate(LXm_PMI_ALL, &m_poly_info);

    draw.SetPart (LXiHITPART_INVIS);
	draw.PushTransform (v, m);

    if (m_poly_info.type == 3)
        draw.Begin (LXiSTROKE_TRIANGLES, draw_rgb, 0.8);
    else if (m_poly_info.type == 4)
        draw.Begin (LXiSTROKE_QUADS, draw_rgb, 0.8);
    else
        return;

    for (auto i = 0u; i < m_poly_info.npol; ++i)
    {
        auto iv = i * m_poly_info.type;
        for (auto j = 0u; j < m_poly_info.type; ++j)
        {
            auto idx = m_poly_info.pols[iv + j] * 3;
            LXx_VCPY(p0, &m_poly_info.vrts[idx]);
            draw.Vertex (p0, LXiSTROKE_ABSOLUTE);
        }
    }
	draw.PopTransform ();
}


void CSpaceTransform::DrawPolygonUV(CLxUser_StrokeDraw& draw, const LXpToolViewEvent* view)
{
    if (m_polygon.test() == false)
        return;
    
    if (view == nullptr)
        return;

    LXtVector p0, draw_rgb;
    LXx_VSET3(draw_rgb, 1.0, 1.0, 0.6);

    LXtMatrix m;
    LXtVector v;

    if (view->type == LXi_VIEWTYPE_3D)
    {
        m_view_matrix.getMatrix3x3(m);
        CLxVector translation = m_view_matrix.getTranslation();
        LXx_VCPY(v, translation.v);
    }
    else
    {
        lx::MatrixIdent(m);
        LXx_VCLR(v);
    }

    draw.SetPart (LXiHITPART_INVIS);
	draw.PushTransform (v, m);

    draw.Begin (LXiSTROKE_LINE_LOOP, draw_rgb, 0.8);

    CLxVector uv0;

    unsigned int nvert;
    m_polygon.VertexCount(&nvert);
    for (unsigned int i = 0; i < nvert; i++)
    {
        LXtPointID vrtID;
        m_polygon.VertexByIndex(i, &vrtID);
        LXtFVector posUV;
        m_polygon.MapEvaluate(m_vmap.ID(), vrtID, posUV);
        CLxVector uv(posUV[0], posUV[1], 0.0);
        if (i == 0)
            uv0 = uv;

        if (view->type == LXi_VIEWTYPE_3D)
        {
            CLxVector pos = (uv - uv0) * m_3d_uv_scale;
            pos *= m_uv_3d_rotate;
            LXx_VCPY(v, pos.v);
        }
        else
        {
            LXx_VCPY(v, uv.v);
        }
        draw.Vertex (v, LXiSTROKE_ABSOLUTE);
    }
	draw.PopTransform ();
}


CLxVector CSpaceTransform::ProjectPointToUV(CLxVector& pos)
{
    return MathUtil::ProjectPointToUV(positions3D[0], positions3D[1], positions3D.back(), pos);
}