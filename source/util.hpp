//
// Utility functions for the mesh processing.
//
#pragma once

#include <lxsdk/lx_log.hpp>
#include <lxsdk/lx_mesh.hpp>
#include <lxsdk/lx_value.hpp>
#include <lxsdk/lxu_math.hpp>
#include <lxsdk/lxvmath.h>
#include <lxsdk/lxu_matrix.hpp>
#include <lxsdk/lxu_quaternion.hpp>
#include <lxsdk/lx_vp.hpp>

#include <vector>
#include <unordered_set>
#include <tuple>

//
// Basic vector math functions.
//
namespace MathUtil {

    static unsigned MaxExtent(const LXtVector v)
    {
        double a = std::abs(v[0]);
        double b = std::abs(v[1]);
        double c = std::abs(v[2]);
        if (a > b && a > c)
            return 0;
        else if (b >= a && b > c)
            return 1;
        else
            return 2;
    }

    template <typename T>
    static bool VectorEqual(const T* a, const T* b, int n)
    {
        while (n--)
            if (lx::Compare(a[n], b[n]))
                return false;

        return true;
    }

    static double AngleVectors(const LXtVector v0, const LXtVector v1)
    {
        double vlen0, vlen1, x;

        vlen0 = LXx_VLEN(v0);
        vlen1 = LXx_VLEN(v1);
        if (vlen0 < lx::Tolerance(vlen0) || vlen1 < lx::Tolerance(vlen1))
            return 0.0;
        x = LXx_VDOT(v0, v1) / vlen0 / vlen1;
        x = LXxCLAMP(x, -1.0, 1.0);
        return std::acos(x);
    }

    static void VectorRotation(LXtMatrix m, const LXtVector v0, const LXtVector v1)
    {
        LXtVector vo;
        double    qq[4];

        if (MathUtil::VectorEqual(v0, v1, LXdND))
        {
            lx::MatrixIdent(m);
            return;
        }

        LXx_VCROSS(vo, v1, v0);
        double theta = AngleVectors(v1, v0);
        if (std::abs(theta) < lx::Tolerance(theta))
        {
            lx::MatrixIdent(m);
            return;
        }
        if (!lx::VectorNormalize(vo))
        {
            qq[0] = 0.0;
            qq[1] = std::sin(theta / 2);
            qq[2] = 0.0;
            qq[3] = std::cos(theta / 2);
        }
        else
        {
            double sint = sin(theta / 2);
            qq[0]       = sint * vo[0];
            qq[1]       = sint * vo[1];
            qq[2]       = sint * vo[2];
            qq[3]       = std::cos(theta / 2);
        }
        CLxQuaternion quat(qq);
        quat.normalize();
        CLxMatrix4 m4 = quat.asMatrix();
        m4.getMatrix3x3(m);
    }

    static double AreaTriangle(const LXtFVector v0, const LXtFVector v1, const LXtFVector v2)
    {
        LXtVector A, B;
        double    AB, AR;

        LXx_VSUB3(A, v1, v0);
        LXx_VSUB3(B, v2, v0);
        AB = LXx_VDOT(A, B);
        AR = LXx_VDOT(A, A) * LXx_VDOT(B, B) - AB * AB;
        if (AR > 0.0)
            return std::sqrt(AR) / 2;
        else
            return 0.0;
    }

    static double AreaTriangle2D(double x0, double y0, double x1, double y1, double x2, double y2)
    {
        double a, b, c, d;

        a = x1 - x0;
        b = y1 - y0;
        c = x2 - x0;
        d = y2 - y0;
        return (a * d - b * c) / 2;
    }

    static double XYAngle(double dx, double dy)
    {
        double a;

        if (dx)
            a = std::atan(dy / dx);
        else if (dy > 0)
            a = LXx_HALFPI;
        else
            a = -LXx_HALFPI;

        if (dx < 0.0)
        {
            if (dy < 0)
                a -= LXx_PI;
            else
                a += LXx_PI;
        }
        return a;
    }

    static double VectorAngle(const LXtVector v0, const LXtVector v1, int normalize)
    {
        double dot = LXx_VDOT (v0, v1);
        if (normalize) {
            dot = dot / (LXx_VLEN (v0) * LXx_VLEN (v1));
        }
        dot = LXxCLAMP(dot, -1.0, 1.0);
        return std::acos (dot);
    }

    #define QX 0
    #define QY 1
    #define QZ 2
    #define QW 3

    static void QuaternionNormalize(const LXtQuaternion q, LXtQuaternion qnorm)
    {
        double mag, c;

        mag = (q[QX] * q[QX] + q[QY] * q[QY] + q[QZ] * q[QZ] + q[QW] * q[QW]);
        if (mag > 0)
        {
            c         = 1.0 / std::sqrt(mag);
            qnorm[QX] = q[QX] * c;
            qnorm[QY] = q[QY] * c;
            qnorm[QZ] = q[QZ] * c;
            qnorm[QW] = q[QW] * c;
        }
        else
        {
            qnorm[QW] = 1.0;
            qnorm[QY] = qnorm[QZ] = qnorm[QX] = 0.0;
        }
    }

    static void QuaternionToMatrix(const LXtQuaternion q, LXtMatrix mat)
    {
        double norm, s, xx, yy, zz, xy, xz, yz, wx, wy, wz;

        norm = q[QX] * q[QX] + q[QY] * q[QY] + q[QZ] * q[QZ] + q[QW] * q[QW];
        s    = (norm > lx::Tolerance(norm)) ? 2.0 / norm : 0;

        xx = q[QX] * q[QX] * s;
        yy = q[QY] * q[QY] * s;
        zz = q[QZ] * q[QZ] * s;
        xy = q[QX] * q[QY] * s;
        xz = q[QX] * q[QZ] * s;
        yz = q[QY] * q[QZ] * s;
        wx = q[QW] * q[QX] * s;
        wy = q[QW] * q[QY] * s;
        wz = q[QW] * q[QZ] * s;

        mat[0][0] = 1.0 - (yy + zz);
        mat[0][1] = xy + wz;
        mat[0][2] = xz - wy;

        mat[1][0] = xy - wz;
        mat[1][1] = 1.0 - (xx + zz);
        mat[1][2] = yz + wx;

        mat[2][0] = xz + wy;
        mat[2][1] = yz - wx;
        mat[2][2] = 1.0 - (xx + yy);
    }

    static void MatrixVectorRotation(LXtMatrix m, const LXtVector v0, const LXtVector v1, double frac = 1.0)
    {
        LXtVector     vo;
        LXtQuaternion quat, qout;
        double        theta, sint;

        if (VectorEqual(v0, v1, LXdND))
        {
            lx::MatrixIdent(m);
            return;
        }

        LXx_VCROSS(vo, v1, v0);
        theta = AngleVectors(v1, v0) * frac;
        if (std::fabs(theta) < lx::Tolerance(theta))
        {
            lx::MatrixIdent(m);
            return;
        }
        if (!lx::VectorNormalize(vo))
        {
            quat[0] = 0.0;
            quat[1] = std::sin(theta / 2);
            quat[2] = 0.0;
            quat[3] = std::cos(theta / 2);
        }
        else
        {
            sint    = sin(theta / 2);
            quat[0] = sint * vo[0];
            quat[1] = sint * vo[1];
            quat[2] = sint * vo[2];
            quat[3] = std::cos(theta / 2);
        }
        QuaternionNormalize(quat, qout);
        QuaternionToMatrix(qout, m);
    }

    static void AxisGetViewPlane(unsigned axis, int* ix, int* iy)
    {
        static const int plx[] = { 2, 0, 0 };
        static const int ply[] = { 1, 2, 1 };

        *ix = plx[axis];
        *iy = ply[axis];
    }

    static bool CrossNormal (LXtVector norm, const LXtVector a1, const LXtVector a2, const LXtVector a3)
    {
        LXtVector a, b;

        LXx_VSUB3 (a, a1, a2);
        LXx_VSUB3 (b, a2, a3);
        LXx_VCROSS (norm, a, b);
        return lx::VectorNormalize (norm);
    }

    static bool CrossNormal (LXtFVector norm, const LXtFVector a1, const LXtFVector a2, const LXtFVector a3)
    {
        LXtFVector a, b;

        LXx_VSUB3 (a, a1, a2);
        LXx_VSUB3 (b, a2, a3);
        LXx_VCROSS (norm, a, b);
        return lx::VectorNormalize (norm);
    }

    static void AxisFromMatrix (const LXtMatrix xfrm, LXtVector vec, int axis)
    {
        for (auto i = 0; i < 3; i++)
            vec[i] = xfrm[i][axis];
    }

    static CLxVector ProjectPointOntoPlane(CLxVector& planeAxis, CLxVector& planeOrigin, CLxVector& pos)
    {
        CLxVector posPlane;
        double distance = LXx_VDOT(pos - planeOrigin, planeAxis);
        posPlane = pos - planeAxis * distance;
        return posPlane;
    }

    static CLxVector ProjectPointToUV(CLxVector& P0, CLxVector& P1, CLxVector& P2, CLxVector& TargetPoint)
    {
        CLxVector axisX = P1 - P0;
        CLxVector axisN = P2 - P0;
        CLxVector normal = axisX.cross(axisN);
        CLxVector axisY = normal.cross(axisX);

        axisX.normalize();
        axisY.normalize();
        normal.normalize();

        // 2. ターゲット点からP0へのベクトル
        CLxVector vec = TargetPoint - P0;

        // 3. 内積をとって2次元座標へ投影
        // 投影成分 = vec・axis
        double u = vec.dot(axisX);
        double v = vec.dot(axisY);

        return CLxVector(u, v, 0.0);
    }

    static CLxMatrix4 MatrixVectorRotation(CLxVector& v0, CLxVector& v1, double frac = 1.0)
    {
        LXtMatrix m;
        MatrixVectorRotation(m, v0.v, v1.v, frac);
        return CLxMatrix4(m);
    }

    static bool PointInTriangle (CLxVector& pos, CLxVector& v0, CLxVector& v1, CLxVector& v2, double& u, double& v)
    {
        CLxVector edge0 = v1 - v0;
        CLxVector edge1 = v2 - v0;
        CLxVector vp = pos - v0;

        double d00 = edge0.dot(edge0);
        double d01 = edge0.dot(edge1);
        double d11 = edge1.dot(edge1);
        double d20 = vp.dot(edge0);
        double d21 = vp.dot(edge1);
        
        double denom = d00 * d11 - d01 * d01;
        
        // 逆行列を計算してu, vを求める
        u = (d11 * d20 - d01 * d21) / denom;
        v = (d00 * d21 - d01 * d20) / denom;

        // 三角形内の判定
        return (u >= 0.0) && (v >= 0.0) && (u + v <= 1.0);
    }

    static void ClosestBarycentricPoint(CLxVector& pos, CLxVector& v0, CLxVector& v1, CLxVector& v2, double& u, double& v) 
    {
        CLxVector ab = v1 - v0;
        CLxVector ac = v2 - v0;
        CLxVector ap = pos - v0;

        double d1 = ab.dot(ap);
        double d2 = ac.dot(ap);
        if (d1 <= 0.0 && d2 <= 0.0) { 
            u = 0.0; v = 0.0; 
            return; 
        }

        CLxVector bp = pos - v1;
        double d3 = ab.dot(bp);
        double d4 = ac.dot(bp);
        if (d3 >= 0.0 && d4 <= d3) { 
            u = 1.0; v = 0.0; 
            return;
        }

        double vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
            u = d1 / (d1 - d3);
            v = 0.0;
            return;
        }

        CLxVector cp = pos - v2;
        double d5 = ab.dot(cp);
        double d6 = ac.dot(cp);
        if (d6 >= 0.0 && d5 <= d6) { 
            u = 0.0; v = 1.0; 
            return; 
        }

        double vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
            u = 0.0;
            v = d2 / (d2 - d6);
            return;
        }

        double va = d3 * d6 - d5 * d4;
        if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
            double denom = 1.0 / ((d4 - d3) + (d5 - d6));
            u = (d4 - d3) * denom;
            v = (d5 - d6) * denom;
            return;
        }

        double denom = 1.0 / (va + vb + vc);
        u = vb * denom;
        v = vc * denom;
    }

    static CLxVector TrianglePoint(CLxVector& v0, CLxVector& v1, CLxVector& v2, double u, double v)
    {
        double w = 1.0 - u - v;
        CLxVector pos = (v0 * w) + (v1 * u) + (v2 * v);
        return pos;
    }

    static CLxVector ClosestTrianglePoint (CLxVector& pos, CLxVector& v0, CLxVector& v1, CLxVector& v2, double& u, double& v)
    {
        CLxVector ab = v1 - v0;
        CLxVector ac = v2 - v0;
        CLxVector ap = pos - v0;

        double d1 = ab.dot(ap);
        double d2 = ac.dot(ap);
        if (d1 <= 0.0 && d2 <= 0.0) {
            // 領域 1: v0 が最接近
            u = 0.0;
            v = 0.0;
            return TrianglePoint(v0, v1, v2, u, v);
        }

        CLxVector bp = pos - v1;
        double d3 = ab.dot(bp);
        double d4 = ac.dot(bp);
        if (d3 >= 0.0 && d4 <= d3) {
            // 領域 2: v1 が最接近
            u = 1.0;
            v = 0.0;
            return TrianglePoint(v0, v1, v2, u, v);
        }

        double vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
            // 領域 3: 辺 v0-v1 が最接近
            u = d1 / (d1 - d3);
            v = 0.0;
            return TrianglePoint(v0, v1, v2, u, v);
        }

        CLxVector cp = pos - v2;
        double d5 = ab.dot(cp);
        double d6 = ac.dot(cp);
        if (d6 >= 0.0 && d5 <= d6) {
            // 領域 6: v2 が最接近
            u = 0.0;
            v = 1.0;
            return TrianglePoint(v0, v1, v2, u, v);
        }

        double vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
            // 領域 4: 辺 v0-v2 が最接近
            u = 0.0;
            v = d2 / (d2 - d6);
            return TrianglePoint(v0, v1, v2, u, v);
        }

        double va = d3 * d6 - d5 * d4;
        if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
            // 領域 5: 辺 v1-v2 が最接近
            double denom = 1.0 / ((d4 - d3) + (d5 - d6));
            u = (d4 - d3) * denom;
            v = (d5 - d6) * denom; // 辺上のパラメータ表現に合わせる
            return TrianglePoint(v0, v1, v2, u, v);
        }

        // 領域 0: 内部 (Point is inside)
        double denom = 1.0 / (va + vb + vc);
        u = vb * denom;
        v = vc * denom;
        return TrianglePoint(v0, v1, v2, u, v);
    }

    static bool IntersectSegmentTriangle (
        CLxVector& p0, CLxVector& p1,
        CLxVector& v0, CLxVector& v1, CLxVector& v2,
        double& outU, double& outV, double& outT) 
    {
        const double EPSILON = 1e-7;
        CLxVector edge1 = v1 - v0;
        CLxVector edge2 = v2 - v0;

        CLxVector rayOrigin = p0;
        CLxVector rayVector = p1 - p0;
        
        CLxVector h = rayVector.cross(edge2);
        double a = edge1.dot(h);

        // a が 0 近傍の場合、線分は三角形と平行
        if (std::abs(a) < EPSILON) {
            return false;
        }

        double f = 1.0 / a;
        CLxVector s = rayOrigin - v0;
        outU = f * s.dot(h);

        // u が範囲外（三角形の外側）
        if (outU < 0.0 || outU > 1.0) {
            return false;
        }

        CLxVector q = s.cross(edge1);
        outV = f * rayVector.dot(q);

        // v が範囲外、または u + v が 1 を超える場合（三角形の外側）
        if (outV < 0.0 || outU + outV > 1.0) {
            return false;
        }

        // 交点までの距離 t を計算
        outT = f * edge2.dot(q);

        // t が 0.0 から 1.0 の間にあれば「線分」上に交点がある
        // （レイとして無限に伸ばす場合は t >= EPSILON のみで判定します）
        if (outT >= 0.0 && outT <= 1.0) {
            return true;
        }

        return false;
    }
};


class CRotationHandle
{
public:
    CRotationHandle()
    {
        m_angle = 0.0;
        m_angleSnap = 0.0;
    }
    void MouseDown(int viewId, const LXtVector center, const LXtVector normal, const LXtVector pos)
    {
        CLxUser_View3DportService s_v3d;
	    s_v3d.View(viewId, m_view3d);
    
        LXx_VCPY (m_center, center);
        LXx_VCPY (m_normal, normal);

        PosToPlane(normal, center, pos, m_down);
        LXx_VCPY (m_move, m_down);

        LXx_VSUB3(m_prev, m_down, m_center);
		lx::VectorNormalize (m_prev);
    
        //printf("[MouseDown] down %f %f m_prev %f %f center %f %f\n",
        //    m_down[0], m_down[1], m_prev[0], m_prev[1], center[0], center[1]);
        LXx_VCPY (m_start, m_prev);
        m_angle = 0.0;
        m_angleSnap = 0.0;
    }

    double MouseMove(const LXtVector pos)
    {
        double angleSnap;
        LXtVector vec, delta, vCross;

		LXx_VCPY (m_move, m_center);
        //m_view3d.To3D(cx, cy, pos, 0);
        PosToPlane(m_normal, m_center, pos, m_move);
    
		LXx_VSUB3 (vec, m_move, m_center);
		lx::VectorNormalize (vec);

		LXx_VSUB3 (delta, vec, m_prev);

		if (LXx_VEQS(delta, 0.0))
			return m_angleSnap;

		double angle = MathUtil::VectorAngle (m_prev, vec, 0);
        //printf("[MouseMove] m_move %f %f vec %f %f angle %f\n",
        //    m_move[0], m_move[1], vec[0], vec[1], angle*LXx_RAD2DEG);

		double minDelta = 1.0 * LXx_DEG2RAD;

		LXx_VCROSS (vCross, m_prev, vec);

		if (LXx_VDOT (m_normal, vCross) < 0.0)
			m_angle -= angle;
		else
			m_angle += angle;

        double value = m_angle / minDelta;
		angleSnap = minDelta * std::ceil (std::abs(value)) * LXxSIGN(value);

		LXx_VCPY (m_prev, vec);

		m_angleSnap = angleSnap;
        return angleSnap;
    }

    void GetAngles(int axis, LXtMatrix xfrm, double* startAngle, double* endAngle)
    {
        LXtVector ref, xRef, vCross, rAxis;
        LXx_VUNIT(ref, (axis+1) % 3);
        lx::MatrixMultiply(xRef, xfrm, ref);
        LXx_VCROSS(vCross, xRef, m_start);

        double sAngle = MathUtil::VectorAngle(m_start, xRef, 0);
        //printf("[GetAngles] m_start %f %f xRef %f %f sAngle %f\n",
        //    m_start[0], m_start[1], xRef[0], xRef[1], sAngle*LXx_RAD2DEG);

        MathUtil::AxisFromMatrix(xfrm, rAxis, axis);

        if (LXx_VDOT(rAxis, vCross) < 0.0)
            sAngle *= -1.0;
        
        double eAngle = sAngle + m_angle;

        if (sAngle > eAngle)
        {
            std::swap(sAngle, eAngle);
        }

        if (startAngle)
            *startAngle = sAngle;
        if (endAngle)
            *endAngle = eAngle;
    }

    void PosToPlane(const LXtVector axis, const LXtVector center, const LXtVector pos, LXtVector posPlane)
    {
        LXtVector eye, del;

        LXx_VCPY(posPlane, pos);
        m_view3d.EyeVector(posPlane, eye);
        LXx_VSUB3(del, center, posPlane);
        double d = LXx_VDOT (del, axis);
        double cost = LXx_VDOT (eye, axis);
        if (std::abs(cost) > 0.0)
        {
            d /= cost;
            LXx_VADDS(posPlane, eye, d);
        }
    }

    double m_angle;
    double m_angleSnap;
    LXtVector m_center, m_normal;
    LXtVector m_down, m_move, m_start, m_prev;

    CLxUser_View3D      m_view3d;
};



