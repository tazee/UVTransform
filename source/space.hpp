//
// SpaceTransform - Transform UV values on UV and 3D spaces.  
//

#pragma once

#include <lxsdk/lx_wrap.hpp>
#include <lxsdk/lx_toolui.hpp>
#include <lxsdk/lx_tool.hpp>
#include <lxsdk/lx_mesh.hpp>
#include <lxsdk/lxw_tool.hpp>
#include <lxsdk/lxw_seltypes.hpp>
#include <lxsdk/lx_value.hpp>
#include <lxsdk/lxu_math.hpp>
#include <lxsdk/lxu_matrix.hpp>
#include <lxsdk/lxu_vector.hpp>
#include <lxsdk/lxu_format.hpp>
#include <lxsdk/lx_draw.hpp>
#include <lxsdk/lxvalue.h>

#include <vector>
#include <unordered_map>
#include <string>
#include <format>

#ifndef LXx_OVERRIDE
#define LXx_OVERRIDE override
#endif

class CSpaceTransform
{
public:
    CSpaceTransform()
    {
    }
    void SetUVMap(LXtMeshMapID vmapID)
    {
        m_vmap.Select(vmapID);
    }
    bool SetPolygon(CLxUser_Mesh& mesh, const LXtMatrix4 matrix, const LXtHitElement* hit);
    void DrawPolygon3D(CLxUser_StrokeDraw& draw, const LXpToolViewEvent* view);
    void DrawPolygonUV(CLxUser_StrokeDraw& draw, const LXpToolViewEvent* view);
    CLxVector PosUVto3D (double u, double v);
    CLxVector Pos3DtoUV (const LXtVector pos3D);

    CLxUser_Mesh m_mesh;
    CLxUser_MeshMap m_vmap;
    CLxUser_Point m_point;
    CLxUser_Polygon m_polygon;
    CLxUser_Edge m_edge;
    CLxUser_MeshService mesh_svc;

    CLxUser_Item m_item;
    CLxUser_Scene m_scene;

    CLxMatrix4 m_matrix;
    //CLxMatrix4 m_3d_uv_matrix;
    //CLxMatrix4 m_uv_3d_matrix;

    CLxBoundingBox m_box3D;
    CLxBoundingBox m_boxUV;

    //LXtPointID m_vrt0, m_vrt1;
    //LXtPolyMeshInfo m_poly_info;

    //CLxMatrix4 m_view_matrix;
    //CLxMatrix4 m_view_matrix_inv;
    //CLxMatrix4 m_uv_3d_rotate;
    double m_3d_uv_scale;

    // the reference triangle
    // v = (v0 * (1-s-t)) + (v1 * s) + (v2 * t)
    unsigned int m_start_index;
    double m_s, m_t;
    CLxVector m_centerUV, m_center3D;

    std::vector<CLxVector> positions3D;
    std::vector<CLxVector> positionsUV;
};
