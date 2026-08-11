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
    if (nvert < 3)
    {
        m_polygon.clear();
        return false;
    }

    CLxBoundingBox boxUV;
    std::unordered_map<LXtPointID,unsigned int> index_map;
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
        index_map[vrtID] = i;
        boxUV.add(positionsUV[i]);
        //printf("[%u] posUV %f %f %f\n", i, posUV[0], posUV[1], posUV[2]);
    }
    m_polyCenterUV = boxUV.center();

    CLxUser_Point upnt;
    upnt.fromMesh(mesh);
    triangles.clear();
    unsigned int count;
    std::array<unsigned int,3> index;
    m_polygon.GenerateTriangles(&count);
    for (auto i = 0; i < count; i++)
    {
        LXtPointID v[3];
        m_polygon.TriangleByIndex(i, &v[0], &v[1], &v[2]);
        for (auto j = 0u; j < 3; j++)
        {
            index[j] = index_map[v[j]];
        }
        triangles.push_back(index);
    }

    m_index = {0u, 1u, 2u};
    m_s = 0.0;
    m_t = 0.0;
    CLxVector hitPos(hit.lPos);
    m_centerUV = positionsUV[0];
    if (hit.isUV)
    {
        for (auto index : triangles)
        {
            LXtPointID vrtID;
            CLxVector pos[3];
            for (auto j = 0u; j < 3; j++)
            {
                pos[j] = positionsUV[index[j]];
            }
            double s, t;
            if (MathUtil::PointInTriangle(hitPos, pos[0], pos[1], pos[2], s, t))
            {
                m_index = index;
                m_s = s;
                m_t = t;
                m_centerUV = hitPos;
                pos[0] = positions3D[m_index[0]];
                pos[1] = positions3D[m_index[1]];
                pos[2] = positions3D[m_index[2]];
                m_center3D = MathUtil::TrianglePoint(pos[0], pos[1], pos[2], s, t);
                break;
            }
        }
    }
    else
    {
        for (auto index : triangles)
        {
            LXtPointID vrtID;
            CLxVector pos[3];
            for (auto j = 0u; j < 3; j++)
            {
                pos[j] = positions3D[index[j]];
            }
            double s, t;
            printf("-- triangle (%u, %u, %u)\n", index[0], index[1], index[2]);
            printf(".  pos0 %f %f %f\n", pos[0][0], pos[0][1], pos[0][2]);
            printf(".  pos1 %f %f %f\n", pos[1][0], pos[1][1], pos[1][2]);
            printf(".  pos2 %f %f %f\n", pos[2][0], pos[2][1], pos[2][2]);
            if (MathUtil::PointInTriangle(hitPos, pos[0], pos[1], pos[2], s, t))
            {
                m_center3D = hitPos;
                m_index = index;
                m_s = s;
                m_t = t;
                printf("** hit triangle s = %f t = %f pos %f %f %f\n", s, t, hit.lPos[0], hit.lPos[1], hit.lPos[2]);
                pos[0] = positionsUV[m_index[0]];
                pos[1] = positionsUV[m_index[1]];
                pos[2] = positionsUV[m_index[2]];
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
        auto j = m_index[i];
        m_box3D.add(positions3D[j]);
        m_boxUV.add(positionsUV[j]);
    }
    auto extent3D = m_box3D.extent();
    auto length3D = (std::max)((std::max)(extent3D[0],extent3D[1]),extent3D[2]);
    auto extentUV = m_boxUV.extent();
    auto lengthUV = (std::max)((std::max)(extentUV[0],extentUV[1]),extentUV[2]);
    m_3d_uv_scale = (lengthUV > 0.0) ? (length3D / lengthUV) : 1.0;
    printf("** m_3d_uv_scale %f (%f / %f)\n", m_3d_uv_scale, length3D, lengthUV);

    return true;
}

bool CSpaceTransform::Test()
{
    if ((positions3D.size () < 3) || (positionsUV.size() < 3))
        return false;
    if (m_polygon.test() == false)
        return false;
    return true;
}


void CSpaceTransform::DrawPolygon3D(CLxUser_StrokeDraw& draw, const LXpToolViewEvent* view)
{
	CLxUser_ShapeDraw shape (draw);

    if (Test() == false)
        return;
    
    if (view == nullptr)
        return;

    if (view->type != LXi_VIEWTYPE_3D)
        return;

    int index;
    m_polygon.Index(&index);
    LXtVector p0, draw_rgb;
    LXx_VSET3(draw_rgb, 1.0, 0.4, 1.0);

    LXtMatrix m;
    LXtVector v;

    m_matrix.getMatrix3x3(m);
    CLxVector translation = m_matrix.getTranslation();
    LXx_VCPY(v, translation.v);
	draw.PushTransform (v, m);

    draw.SetPart (LXiHITPART_INVIS);

    draw.BeginW (LXiSTROKE_LINE_LOOP, draw_rgb, 0.8, 3.0);
    draw.Vertex (positions3D[m_index[0]], LXiSTROKE_ABSOLUTE);
    draw.Vertex (positions3D[m_index[1]], LXiSTROKE_ABSOLUTE);
    draw.Vertex (positions3D[m_index[2]], LXiSTROKE_ABSOLUTE);

	draw.PopTransform ();

    /*
    LXx_VSET3(draw_rgb, 0.6, 0.6, 1.0);
    LXtVector rad;
    LXx_VSET(rad, 0.2);
    CLxVector cent = PosUVto3D(m_polyCenterUV[0], m_polyCenterUV[1]);
	draw.PushTransform (cent.v, m);
    shape.PreciseHandle (draw_rgb, 0.8, v, rad, LXiSTROKE_SCREEN);
	draw.PopTransform ();
    */
}


void CSpaceTransform::DrawPolygonUV(CLxUser_StrokeDraw& draw, const LXpToolViewEvent* view)
{
	CLxUser_ShapeDraw shape (draw);

    if (Test() == false)
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

    draw.Begin (LXiSTROKE_LINE_LOOP, draw_rgb, 0.8);

    for (auto i = 0u; i < 3; i++)
    {
        LXtPointID vrtID;
        m_polygon.VertexByIndex(m_index[i], &vrtID);
        LXtFVector posUV;
        m_polygon.MapEvaluate(m_vmap.ID(), vrtID, posUV);
        CLxVector uv(posUV[0], posUV[1], 0.0);
        draw.Vertex (uv.v, LXiSTROKE_ABSOLUTE);
    }

    LXx_VSET3(draw_rgb, 0.6, 0.6, 1.0);
    LXtVector rad;
    LXx_VSET(rad, 0.2);

	//draw.PushTransform (m_polyCenterUV.v, m);
    //shape.PreciseHandle (draw_rgb, 0.8, v, rad, LXiSTROKE_SCREEN);
	//draw.PopTransform ();
}


CLxVector CSpaceTransform::PosUVto3D (double u, double v, bool worldSpace)
{
    if (Test() == false)
        return CLxVector();
    CLxVector hitPos(u, v, 0.0);
    CLxVector pos0 = positionsUV[m_index[0]];
    CLxVector pos1 = positionsUV[m_index[1]];
    CLxVector pos2 = positionsUV[m_index[2]];
    double s, t;
    CLxVector pos3D;
    if (MathUtil::PointInTriangle(hitPos, pos0, pos1, pos2, s, t) == true)
    {
        pos0 = positions3D[m_index[0]];
        pos1 = positions3D[m_index[1]];
        pos2 = positions3D[m_index[2]];
        pos3D = MathUtil::TrianglePoint(pos0, pos1, pos2, s, t);
    }
    else
    {
        const double c = 1.0 / 3.0;
        CLxVector cenUV = MathUtil::TrianglePoint(pos0, pos1, pos2, c, c);
        double w;
        MathUtil::IntersectSegmentTriangle(cenUV, hitPos, pos0, pos1, pos2, s, t, w);
        CLxVector sideUV = MathUtil::TrianglePoint(pos0, pos1, pos2, s, t);
        double l = (sideUV - cenUV).length();
        double f = (l > 0.0) ? ((hitPos - cenUV).length() / l) : 0.0;
        pos0 = positions3D[m_index[0]];
        pos1 = positions3D[m_index[1]];
        pos2 = positions3D[m_index[2]];
        CLxVector cen3D = MathUtil::TrianglePoint(pos0, pos1, pos2, c, c);
        CLxVector side3D = MathUtil::TrianglePoint(pos0, pos1, pos2, s, t);
        pos3D = (side3D - cen3D) * f + cen3D;
    }
    if (worldSpace == true)
        pos3D = pos3D * m_matrix;
    return pos3D;
}


CLxVector CSpaceTransform::FindPos3D (double u, double v, bool worldSpace)
{
    if (Test() == false)
        return CLxVector();

    CLxVector hitPos(u, v, 0.0);

    CLxUser_Polygon polygon;
    CLxUser_Point point;
    polygon.fromMesh(m_mesh);
    point.fromMesh(m_mesh);

    LXtFVector pos3d, posUV;
    LXtPointID vrtID;
    LXx_VCLR(posUV);
    m_polygon.VertexByIndex(m_index[0], &vrtID);
    point.Select(vrtID);
    m_polygon.MapEvaluate(m_vmap.ID(), vrtID, posUV);
    CLxVector pos0 = CLxVector(posUV);
    m_polygon.VertexByIndex(m_index[1], &vrtID);
    point.Select(vrtID);
    m_polygon.MapEvaluate(m_vmap.ID(), vrtID, posUV);
    CLxVector pos1 = CLxVector(posUV);
    m_polygon.VertexByIndex(m_index[2], &vrtID);
    point.Select(vrtID);
    m_polygon.MapEvaluate(m_vmap.ID(), vrtID, posUV);
    CLxVector pos2 = CLxVector(posUV);

    double s, t;
    CLxVector pos3D;
    if (MathUtil::PointInTriangle(hitPos, pos0, pos1, pos2, s, t) == true)
    {
        pos0 = positions3D[m_index[0]];
        pos1 = positions3D[m_index[1]];
        pos2 = positions3D[m_index[2]];
        pos3D = MathUtil::TrianglePoint(pos0, pos1, pos2, s, t);
    }
    else
    {
        for (auto index : triangles)
        {
            if (index == m_index)
                continue;
            m_polygon.VertexByIndex(index[0], &vrtID);
            point.Select(vrtID);
            m_polygon.MapEvaluate(m_vmap.ID(), vrtID, posUV);
            pos0 = CLxVector(posUV);
            m_polygon.VertexByIndex(index[1], &vrtID);
            point.Select(vrtID);
            m_polygon.MapEvaluate(m_vmap.ID(), vrtID, posUV);
            pos1 = CLxVector(posUV);
            m_polygon.VertexByIndex(index[2], &vrtID);
            point.Select(vrtID);
            m_polygon.MapEvaluate(m_vmap.ID(), vrtID, posUV);
            pos2 = CLxVector(posUV);
            if (MathUtil::PointInTriangle(hitPos, pos0, pos1, pos2, s, t) == true)
            {
                pos0 = positions3D[index[0]];
                pos1 = positions3D[index[1]];
                pos2 = positions3D[index[2]];
                pos3D = MathUtil::TrianglePoint(pos0, pos1, pos2, s, t);
                if (worldSpace == true)
                    pos3D = pos3D * m_matrix;
                return pos3D;
            }
        }
/*
        unsigned int count;
        m_mesh.PolygonCount(&count);
        for (auto i = 0u; i < count; i++)
        {
            unsigned int ntri;
            polygon.SelectByIndex(i);
            if (polygon == m_polygon)
                continue;
            polygon.GenerateTriangles(&ntri);
            for (auto j = 0; j < ntri; j++)
            {
                LXtPointID v[3];
                polygon.TriangleByIndex(j, &v[0], &v[1], &v[2]);
                polygon.MapEvaluate(m_vmap.ID(), v[0], posUV);
                pos0  = CLxVector(posUV);
                polygon.MapEvaluate(m_vmap.ID(), v[1], posUV);
                pos1  = CLxVector(posUV);
                polygon.MapEvaluate(m_vmap.ID(), v[2], posUV);
                pos2  = CLxVector(posUV);
                if (MathUtil::PointInTriangle(hitPos, pos0, pos1, pos2, s, t) == true)
                {
                    point.Select(v[0]);
                    point.Pos(pos3d);
                    pos0  = CLxVector(pos3d);
                    point.Select(v[1]);
                    point.Pos(pos3d);
                    pos1  = CLxVector(pos3d);
                    point.Select(v[2]);
                    point.Pos(pos3d);
                    pos2  = CLxVector(pos3d);
                    pos3D = MathUtil::TrianglePoint(pos0, pos1, pos2, s, t);
                    if (worldSpace == true)
                        pos3D = pos3D * m_matrix;
                    return pos3D;
                }
            }
        }
*/
        m_polygon.VertexByIndex(m_index[0], &vrtID);
        point.Select(vrtID);
        m_polygon.MapEvaluate(m_vmap.ID(), vrtID, posUV);
        pos0 = CLxVector(posUV);
        m_polygon.VertexByIndex(m_index[1], &vrtID);
        point.Select(vrtID);
        m_polygon.MapEvaluate(m_vmap.ID(), vrtID, posUV);
        pos1 = CLxVector(posUV);
        m_polygon.VertexByIndex(m_index[2], &vrtID);
        point.Select(vrtID);
        m_polygon.MapEvaluate(m_vmap.ID(), vrtID, posUV);
        pos2 = CLxVector(posUV);
        const double c = 1.0 / 3.0;
        CLxVector cenUV = MathUtil::TrianglePoint(pos0, pos1, pos2, c, c);
        MathUtil::IntersectSegmentTriangle2D(cenUV, hitPos, pos0, pos1, pos2, s, t);
        //printf("** FindPos3D s %f t %f\n", s, t);
        CLxVector sideUV = MathUtil::TrianglePoint(pos0, pos1, pos2, s, t);
        double l = (sideUV - cenUV).length();
        double f = (l > 0.0) ? ((hitPos - cenUV).length() / l) : 0.0;
        pos0 = positions3D[m_index[0]];
        pos1 = positions3D[m_index[1]];
        pos2 = positions3D[m_index[2]];
        CLxVector cen3D = MathUtil::TrianglePoint(pos0, pos1, pos2, c, c);
        CLxVector side3D = MathUtil::TrianglePoint(pos0, pos1, pos2, s, t);
        pos3D = (side3D - cen3D) * f + cen3D;
    }
    if (worldSpace == true)
        pos3D = pos3D * m_matrix;
    return pos3D;
}


CLxVector CSpaceTransform::Pos3DtoUV (const LXtVector pos3D)
{
    if (Test() == false)
        return CLxVector(0,0,0);
    CLxVector hitPos(pos3D);
    CLxVector pos0 = positions3D[m_index[0]];
    CLxVector pos1 = positions3D[m_index[1]];
    CLxVector pos2 = positions3D[m_index[2]];
    double s, t;
    CLxVector posUV;
    hitPos = hitPos * m_matrix.inverse();
    if (MathUtil::PointInTriangle(hitPos, pos0, pos1, pos2, s, t) == true)
    {
        pos0 = positionsUV[m_index[0]];
        pos1 = positionsUV[m_index[1]];
        pos2 = positionsUV[m_index[2]];
        posUV = MathUtil::TrianglePoint(pos0, pos1, pos2, s, t);
    }
    else
    {
        const double c = 1.0 / 3.0;
        CLxVector cen3D = MathUtil::TrianglePoint(pos0, pos1, pos2, c, c);
        double w;
        MathUtil::IntersectSegmentTriangle(cen3D, hitPos, pos0, pos1, pos2, s, t, w);
        CLxVector side3D = MathUtil::TrianglePoint(pos0, pos1, pos2, s, t);
        double l = (side3D - cen3D).length();
        double f = (l > 0.0) ? ((hitPos - cen3D).length() / l) : 0.0;
        pos0 = positionsUV[m_index[0]];
        pos1 = positionsUV[m_index[1]];
        pos2 = positionsUV[m_index[2]];
        CLxVector cenUV = MathUtil::TrianglePoint(pos0, pos1, pos2, c, c);
        CLxVector sideUV = MathUtil::TrianglePoint(pos0, pos1, pos2, s, t);
        posUV = (sideUV - cenUV) * f + cenUV;
    }
    return posUV;
}


CLxVector CSpaceTransform::ProjectPos3D (const LXtVector pos3D, const LXtVector dir)
{
    if (Test() == false)
    {
        return CLxVector(pos3D);
    }
    CLxVector rayOrigin(pos3D);
    CLxVector rayVector(dir);
    CLxVector pos0 = positions3D[m_index[0]] * m_matrix;
    CLxVector pos1 = positions3D[m_index[1]] * m_matrix;
    CLxVector pos2 = positions3D[m_index[2]] * m_matrix;
    CLxVector outPos;
    double t;
    if (MathUtil::intersectPlaneAndRay(pos0, pos1, pos2, rayOrigin, rayVector, outPos, t))
    {
        return outPos;
    }
    else
        return CLxVector(pos3D);
}


CLxVector CSpaceTransform::TriangleNormal (bool worldSpace)
{
    if (Test() == false)
        return CLxVector(0,0,0);
    CLxVector pos0 = positions3D[m_index[0]];
    CLxVector pos1 = positions3D[m_index[1]];
    CLxVector pos2 = positions3D[m_index[2]];
    CLxVector vec0 = pos1 - pos0;
    CLxVector vec1 = pos2 - pos0;
    CLxVector norm = vec0.cross(vec1);
    norm.normalize();
    if (worldSpace == true)
        norm = norm * m_matrix.asRotateMatrix();
    return norm;
}


CLxVector CSpaceTransform::TriangleCenter (bool worldSpace)
{
    if (Test() == false)
        return CLxVector(0,0,0);
    const double c = 1.0 / 3.0;
    CLxVector pos0 = positions3D[m_index[0]];
    CLxVector pos1 = positions3D[m_index[1]];
    CLxVector pos2 = positions3D[m_index[2]];
    CLxVector cen3D = MathUtil::TrianglePoint(pos0, pos1, pos2, c, c);
    if (worldSpace == true)
        cen3D = cen3D * m_matrix;
    return cen3D;
}

class BoxVisitor : public CLxImpl_AbstractVisitor
{
public:
    LxResult Evaluate()
    {
        unsigned int nvert;
        int index;
        m_poly.VertexCount(&nvert);
        m_poly.Index(&index);
        for (auto i = 0u; i < nvert; i++)
        {
            LXtPointID vertID;
            m_poly.VertexByIndex(i, &vertID);
            m_vert.Select(vertID);
            unsigned index;
            m_vert.Index(&index);
            if (m_vert.TestMarks(m_mark_pick) == LXe_FALSE)
                continue;
            float value[2], new_value[2];
            if (m_poly.MapValue(m_vmap.ID(), vertID, value) == LXe_OK)
            {
                if (m_vert.TestMarks(m_mark_pick) == LXe_TRUE)
                {
                    CLxVector posUV(value[0], value[1], 0.0);
                    m_boxUV.add(posUV);
                }
            }
            else
            {
                if (m_vert.MapValue(m_vmap.ID(), value) == LXe_OK)
                {
                    CLxVector posUV(value[0], value[1], 0.0);
                    m_boxUV.add(posUV);
                }
            }
        }
        return LXe_OK;
    }

    CLxUser_Mesh    m_mesh;
    CLxUser_Polygon m_poly;
    CLxUser_Point   m_vert;
    CLxUser_Edge    m_edge;
    CLxUser_MeshMap m_vmap;
    LXtMarkMode     m_mark_pick;
    LXtMarkMode     m_mark_done;
    CLxBoundingBox  m_boxUV;
};

CLxVector CSpaceTransform::SelectionCenterUV (CLxUser_Subject2Packet& subject)
{
    if (Test() == false)
        return CLxVector();
    CLxUser_LayerScan  scan;

    subject.BeginScan(LXf_LAYERSCAN_ACTIVE | LXf_LAYERSCAN_MARKALL, scan);

    BoxVisitor vis;
    vis.m_mesh = m_mesh;
    vis.m_vmap = m_vmap;
    vis.m_poly.fromMesh(m_mesh);
    vis.m_vert.fromMesh(m_mesh);
    vis.m_edge.fromMesh(m_mesh);
    vis.m_mark_pick = mesh_svc.SetMode(LXsMARK_SELECT);
    if (subject.Type() == LXiSEL_POLYGON)
        vis.m_poly.Enum(&vis, vis.m_mark_pick);
    else
        vis.m_poly.Enum(&vis, LXiMARK_ANY);

    m_polyCenterUV = vis.m_boxUV.center();
    return vis.m_boxUV.center();
}


void CSpaceTransform::UpdatePositionUV()
{
    CLxBoundingBox boxUV;
    for (auto i = 0u; i < positionsUV.size(); i++)
    {
        LXtPointID vrtID;
        m_polygon.VertexByIndex(i, &vrtID);
        LXtFVector posUV;
        m_point.Select(vrtID);
        LXx_VCLR(posUV);
        m_polygon.MapEvaluate(m_vmap.ID(), vrtID, posUV);
        positionsUV[i] = CLxVector(posUV);
        boxUV.add(positionsUV[i]);
    }
    m_polyCenterUV = boxUV.center();
}

CLxMatrix4 CSpaceTransform::MatrixUVto3D()
{
    double c = 1.0 / 3.0;
    CLxVector pos0 = positionsUV[m_index[0]];
    CLxVector pos1 = positionsUV[m_index[1]];
    CLxVector pos2 = positionsUV[m_index[2]];
    CLxVector cenUV = MathUtil::TrianglePoint(pos0, pos1, pos2, c, c);
    CLxVector posU(cenUV[0] + 0.01, cenUV[1], cenUV[2]);
    CLxVector posV(cenUV[0], cenUV[1] + 0.01, cenUV[2]);
    pos0 = positions3D[m_index[0]] * m_matrix;
    pos1 = positions3D[m_index[1]] * m_matrix;
    pos2 = positions3D[m_index[2]] * m_matrix;
    CLxVector cen3D = MathUtil::TrianglePoint(pos0, pos1, pos2, c, c);
    CLxVector vecX = PosUVto3D(posU[0], posU[1], true);
    CLxVector vecY = PosUVto3D(posV[0], posV[1], true);
    vecX = vecX - cen3D;
    vecY = vecY - cen3D;
    vecX.normalize();
    vecY.normalize();
    CLxVector vecZ = vecX.cross(vecY);
    vecZ.normalize();
    LXtMatrix m;
    LXx_VSET3(m[0], vecX[0], vecY[0], vecZ[0]);
    LXx_VSET3(m[1], vecX[1], vecY[1], vecZ[1]);
    LXx_VSET3(m[2], vecX[2], vecY[2], vecZ[2]);
    return CLxMatrix4(m);
}