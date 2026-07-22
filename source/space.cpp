//
// AutoPartByDistance - A plugin to group polygon parts that are close together. 
//

#include "space.hpp"
#include "lxsdk/lxdraw.h"
#include "util.hpp"

bool CSpaceTransform::SetPolygon(CLxUser_Mesh& mesh, const LXtMatrix4 matrix, LXtHitElement& hit)
{
    m_polygon.clear();
    m_edge.clear();
    m_point.clear();

    m_mesh = mesh;
    m_matrix = CLxMatrix4(matrix);

    m_point.fromMesh(mesh);
    m_polygon.fromMesh(mesh);
    m_edge.fromMesh(mesh);

    LXtPolygonID polyID = nullptr;
    if (hit.type == LXiSEL_VERTEX)
    {
        m_point.Select(hit.vrt);
        m_point.PolygonByIndex(0, &polyID);
    }
    else if (hit.type == LXiSEL_EDGE)
    {
        m_edge.Select(hit.edge);
        m_edge.PolygonByIndex(0, &polyID);
    }
    else if (hit.type == LXiSEL_POLYGON)
    {
        polyID = hit.pol;
    }
    else if (hit.type == lx::StringID4("DISC"))
    {
        polyID = hit.pol;
        if (hit.vrt)
            m_point.Select(hit.vrt);
    }
    else if (hit.type == lx::StringID4("DSED"))
    {
        polyID = hit.pol;
        if (hit.edge)
            m_edge.Select(hit.edge);
    }

    if (!polyID)
        return false;

    m_item.set(hit.item);
    m_scene.from(m_item);

    m_polygon.Select(polyID);
    unsigned int nvert;
    m_polygon.VertexCount(&nvert);
    printf("SetPolygon: vrt=%p edge=%p pol=%p, nvert=%u\n", hit.vrt, hit.edge, hit.pol, nvert);
    if (nvert < 3)
    {
        m_polygon.clear();
        return false;
    }

    positions3D.resize(nvert);
    positionsUV.resize(nvert);
    for (unsigned int i = 0; i < nvert; i++)
    {
        LXtPointID vrtID;
        m_polygon.VertexByIndex(i, &vrtID);
        LXtFVector pos3d, posUV;
        m_point.Select(vrtID);
        m_point.Pos(pos3d);
        positions3D[i] = CLxVector(pos3d);
        LXx_VCLR(posUV);
        m_polygon.MapEvaluate(m_vmap.ID(), vrtID, posUV);
        positionsUV[i] = CLxVector(posUV);
        //printf("[%u] posUV %f %f %f\n", i, posUV[0], posUV[1], posUV[2]);
    }

    m_start_index = 0;
    m_s = 0.0;
    m_t = 0.0;
    CLxVector hitPos(hit.lPos);
    m_centerUV = positionsUV[0];
    if (hit.isUV)
    {
        for (auto i = 0u; i < positionsUV.size(); i++)
        {
            LXtPointID vrtID;
            CLxVector pos[3];
            for (auto j = 0u; j < 3; j++)
            {
                pos[j] = positionsUV[(i + j) % positionsUV.size()];
            }
            double s, t;
            if (MathUtil::PointInTriangle(hitPos, pos[0], pos[1], pos[2], s, t))
            {
                m_start_index = i;
                m_s = s;
                m_t = t;
                m_centerUV = hitPos;
                break;
            }
        }
    }
    else
    {
        for (auto i = 0u; i < positions3D.size(); i++)
        {
            LXtPointID vrtID;
            CLxVector pos[3];
            for (auto j = 0u; j < 3; j++)
            {
                pos[j] = positions3D[(i + j) % positions3D.size()];
            }
            double s, t;
            if (MathUtil::PointInTriangle(hitPos, pos[0], pos[1], pos[2], s, t))
            {
                m_start_index = i;
                m_s = s;
                m_t = t;
                printf("** hit triangle [%u] s = %f t = %f pos %f %f %f\n", i, s, t, hit.lPos[0], hit.lPos[1], hit.lPos[2]);
                pos[0] = positionsUV[i];
                pos[1] = positionsUV[(i + 1) % nvert];
                pos[2] = positionsUV[(i + 2) % nvert];
                m_centerUV = MathUtil::TrianglePoint(pos[0], pos[1], pos[2], s, t);
                printf("-- 3D %f %f %f UV %f %f\n", hitPos[0], hitPos[1], hitPos[2], m_centerUV[0], m_centerUV[1]);
                break;
            }
        }
    }

    m_box3D.clear();
    m_boxUV.clear();
    for (auto i = 0; i < 3; i++)
    {
        auto j = (i + m_start_index) % positions3D.size();
        m_box3D.add(positions3D[j]);
        m_boxUV.add(positionsUV[j]);
    }
    auto extent3D = m_box3D.extent();
    auto length3D = std::max(std::max(extent3D[0],extent3D[1]),extent3D[2]);
    auto extentUV = m_boxUV.extent();
    auto lengthUV = std::max(std::max(extentUV[0],extentUV[1]),extentUV[2]);
    m_3d_uv_scale = (lengthUV > 0.0) ? (length3D / lengthUV) : 1.0;
    printf("** m_3d_uv_scale %f (%f / %f)\n", m_3d_uv_scale, length3D, lengthUV);

    return true;
}


void CSpaceTransform::DrawPolygon3D(CLxUser_StrokeDraw& draw, const LXpToolViewEvent* view)
{
    if (m_polygon.test() == false)
        return;
    
    if (view == nullptr)
        return;

    if (view->type != LXi_VIEWTYPE_3D)
        return;

    int index;
    m_polygon.Index(&index);
    //printf("DrawPolygon: (%d) npol=%u, type=%u\n", index, m_poly_info.npol, m_poly_info.type);
    LXtVector p0, draw_rgb;
    LXx_VSET3(draw_rgb, 1.0, 0.4, 1.0);

    LXtMatrix m;
    LXtVector v;

    m_matrix.getMatrix3x3(m);
    CLxVector translation = m_matrix.getTranslation();
    LXx_VCPY(v, translation.v);

    //m_polygon.Tessellate(LXm_PMI_ALL, &m_poly_info);

    draw.SetPart (LXiHITPART_INVIS);
	draw.PushTransform (v, m);

    draw.BeginW (LXiSTROKE_LINE_LOOP, draw_rgb, 0.8, 3.0);
    draw.Vertex (positions3D[m_start_index], LXiSTROKE_ABSOLUTE);
    draw.Vertex (positions3D[(m_start_index + 1) % positions3D.size()], LXiSTROKE_ABSOLUTE);
    draw.Vertex (positions3D[(m_start_index + 2) % positions3D.size()], LXiSTROKE_ABSOLUTE);

	draw.PopTransform ();
}


void CSpaceTransform::DrawPolygonUV(CLxUser_StrokeDraw& draw, const LXpToolViewEvent* view)
{
    if (m_polygon.test() == false)
        return;
    
    if (view == nullptr)
        return;

    if (view->type != LXi_VIEWTYPE_UV)
        return;

    LXtVector p0, draw_rgb;
    LXx_VSET3(draw_rgb, 1.0, 1.0, 0.6);

    LXtMatrix m;
    LXtVector v;

    lx::MatrixIdent(m);
    LXx_VCLR(v);

    draw.SetPart (LXiHITPART_INVIS);
	draw.PushTransform (v, m);

    draw.Begin (LXiSTROKE_LINE_LOOP, draw_rgb, 0.8);

    unsigned int nvert;
    m_polygon.VertexCount(&nvert);
    for (unsigned int i = m_start_index; i < m_start_index + 3; i++)
    {
        LXtPointID vrtID;
        m_polygon.VertexByIndex(i % nvert, &vrtID);
        LXtFVector posUV;
        m_polygon.MapEvaluate(m_vmap.ID(), vrtID, posUV);
        CLxVector uv(posUV[0], posUV[1], 0.0);
        draw.Vertex (uv.v, LXiSTROKE_ABSOLUTE);
    }
	draw.PopTransform ();
}


CLxVector CSpaceTransform::PosUVto3D (double u, double v)
{
    CLxVector hitPos(u, v, 0.0);
    CLxVector pos0 = positionsUV[m_start_index];
    CLxVector pos1 = positionsUV[(m_start_index + 1) % positionsUV.size()];
    CLxVector pos2 = positionsUV[(m_start_index + 2) % positionsUV.size()];
    double s, t;
    if (MathUtil::PointInTriangle(hitPos, pos0, pos1, pos2, s, t) == false)
    {
        MathUtil::ClosestTrianglePoint(hitPos, pos0, pos1, pos2, s, t);
    }
    pos0 = positions3D[m_start_index];
    pos1 = positions3D[(m_start_index + 1) % positions3D.size()];
    pos2 = positions3D[(m_start_index + 2) % positions3D.size()];
    CLxVector pos3D = MathUtil::TrianglePoint(pos0, pos1, pos2, s, t);
    return pos3D;
}


CLxVector CSpaceTransform::Pos3DtoUV (const LXtVector pos3D)
{
    CLxVector hitPos(pos3D);
    CLxVector pos0 = positions3D[m_start_index];
    CLxVector pos1 = positions3D[(m_start_index + 1) % positions3D.size()];
    CLxVector pos2 = positions3D[(m_start_index + 2) % positions3D.size()];
    double s, t;
    if (MathUtil::PointInTriangle(hitPos, pos0, pos1, pos2, s, t) == false)
    {
        MathUtil::ClosestTrianglePoint(hitPos, pos0, pos1, pos2, s, t);
    }
    pos0 = positionsUV[m_start_index];
    pos1 = positionsUV[(m_start_index + 1) % positionsUV.size()];
    pos2 = positionsUV[(m_start_index + 2) % positionsUV.size()];
    CLxVector posUV = MathUtil::TrianglePoint(pos0, pos1, pos2, s, t);
    return posUV;
}


CLxVector CSpaceTransform::TriangleNormal ()
{
    CLxVector pos0 = positions3D[m_start_index];
    CLxVector pos1 = positions3D[(m_start_index + 1) % positions3D.size()];
    CLxVector pos2 = positions3D[(m_start_index + 2) % positions3D.size()];
    CLxVector vec0 = pos1 - pos0;
    CLxVector vec1 = pos2 - pos0;
    CLxVector norm = vec0.cross(vec1);
    norm.normalize();
    return norm;
}