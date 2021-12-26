#ifndef MESH_HEADER_GUARD
#define MESH_HEADER_GUARD

#include <BasicLinearAlgebra.h>
#include <KeepMeAlive.h>
#include "Tridiagonal.h"
#include "IntegrationPoints.h"
#include "print_util.h"

using namespace prnt;

class Input;

/*
    Represents the one-dimensional FEM mesh.
    Stores all the nodes and has methods that generate their coordinates
    and preform the integration.
*/
template<int nNodes>
class Mesh {
public:
    struct Node {
        float t, r;
    };

    Node nodes[nNodes];

    void generate(float t0, float elemSize) {
        float r = 0;
        for (auto& node : nodes) {
            node.t = t0;
            node.r = r;

            r += elemSize;
        }
    }

    void integrateStep(float dTau, float rMax, float tAmbient, const Input& input) {
        TridiagMat<nNodes> H;
        BLA::Matrix<nNodes> P;

        H.Fill(0);
        P.Fill(0);

        for (int i = 0; i < nNodes-1; i++) {
            BLA::Matrix<2,2> Hlocal;
            BLA::Matrix<2> Plocal;
            Hlocal.Fill(0);
            Plocal.Fill(0);

            auto& nodeI = nodes[i];
            auto& nodeJ = nodes[i+1];

            float dR = fabs(nodeI.r - nodeJ.r);

            float alphaAir = (i == nNodes-2) ? input.alphaAir : 0;

            for (unsigned j = 0; j <= input.integrationScheme; j++) {
                const auto& intPoint = IntegrationPoints::get(input.integrationScheme, j);

                float n0 = 0.5f * (1 - intPoint.xi);
                float n1 = 0.5f * (1 + intPoint.xi);

                float r = nodeI.r * n0 + nodeJ.r * n1;
                float t = nodeI.t * n0 + nodeJ.t * n1;

                auto tmp = input.C * input.Ro * dR * r * intPoint.weight;

                Hlocal(0,0) += input.K*r*intPoint.weight/dR + tmp*n0*n0/dTau;
                Hlocal(0,1) += -input.K*r*intPoint.weight/dR + tmp*n0*n1/dTau;
                Hlocal(1,0) = Hlocal(0,1);
                Hlocal(1,1) += input.K*r*intPoint.weight/dR + tmp*n1*n1/dTau + 2.f*alphaAir*rMax;

                Plocal(0) += tmp*t*n0/dTau;
                Plocal(1) += tmp*t*n1/dTau + 2.f*alphaAir*rMax*tAmbient;
                watchdogTimer.reset();
            }

            P.template Submatrix<2, 1>(i, 0) += Plocal;
            H.template Submatrix<2, 2>(i, i) += Hlocal;
        }

        auto decomposition = BLA::LUDecompose(H);
        auto t = BLA::LUSolve(decomposition, P);
        watchdogTimer.reset();

        for (int i = 0; i < nNodes; i++)
            nodes[i].t = t(i);
    }
};

#endif
