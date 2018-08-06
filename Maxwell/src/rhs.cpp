#include "rhs.h"

using namespace std;
using namespace nlsm;

/*----------------------------------------------------------------------;
 *
 * RHS for non-linear sigma model
 *
 *----------------------------------------------------------------------*/
void nlsmRhs(double **unzipVarsRHS, const double **uZipVars,
             const unsigned int& offset,
             const double *pmin, const double *pmax, const unsigned int *sz,
             const unsigned int& bflag)
{



  const double *chi = &uZipVars[VAR::U_CHI][offset];
  const double *phi = &uZipVars[VAR::U_PHI][offset];

  double *chi_rhs = &unzipVarsRHS[VAR::U_CHI][offset];
  double *phi_rhs = &unzipVarsRHS[VAR::U_PHI][offset];

  const unsigned int nx = sz[0];
  const unsigned int ny = sz[1];
  const unsigned int nz = sz[2];

  double hx = (pmax[0] - pmin[0]) / (nx - 1);
  double hy = (pmax[1] - pmin[1]) / (ny - 1);
  double hz = (pmax[2] - pmin[2]) / (nz - 1);

  int idx[3];

  unsigned int n = sz[0]*sz[1]*sz[2];

nlsm::timer::t_deriv.start();

  double *grad_0_chi = new double [n];
  double *grad_1_chi = new double [n];
  double *grad_2_chi = new double [n];

  double *grad_0_phi = new double [n];
  double *grad_1_phi = new double [n];
  double *grad_2_phi = new double [n];

  double *grad2_0_0_chi = new double [n];
  double *grad2_1_1_chi = new double [n];
  double *grad2_2_2_chi = new double [n];

  deriv_xx(grad2_0_0_chi, chi, hx, sz, bflag);
  deriv_yy(grad2_1_1_chi, chi, hy, sz, bflag);
  deriv_zz(grad2_2_2_chi, chi, hz, sz, bflag);

nlsm::timer::t_deriv.stop();

  register double x;
  register double y;
  register double z;
  register unsigned int pp;

  double r;
  double eta;

  //cout << "begin loop" << endl;
  for (unsigned int k = 3; k < nz-3; k++) {
      z = pmin[2] + k*hz;

    for (unsigned int j = 3; j < ny-3; j++) {
       y = pmin[1] + j*hy;

      for (unsigned int i = 3; i < nx-3; i++) {
         x = pmin[0] + i*hx;
         pp = i + nx*(j + ny*k);
         r= sqrt(x*x + y*y + z*z);

#if NONLINEAR
         if (r > 1.0e-17) {


nlsm::timer::t_rhs.start();

#include "nlsm_eqs.cpp"

nlsm::timer::t_rhs.stop();
         } else {
           chi_rhs[pp] = 0.0;
           phi_rhs[pp] = 0.0;
         }
#else
         phi_rhs[pp] = grad2_0_0_chi[pp]+grad2_1_1_chi[pp]+grad2_2_2_chi[pp];
//--
         chi_rhs[pp] = phi[pp];
#endif



       /* debugging */
        unsigned int qi = 46 - 1;
        unsigned int qj = 10 - 1;
        unsigned int qk = 60 - 1;
        unsigned int qidx = qi + nx*(qj + ny*qk);
        if (0 && qidx == pp) {
          std::cout << ".... end OPTIMIZED debug stuff..." << std::endl;
        }

      }
    }
  }


  if (bflag != 0) {

    nlsm::timer::t_bdyc.start();

    deriv_x(grad_0_chi, chi, hx, sz, bflag);
    deriv_y(grad_1_chi, chi, hy, sz, bflag);
    deriv_z(grad_2_chi, chi, hz, sz, bflag);

    deriv_x(grad_0_phi, phi, hx, sz, bflag);
    deriv_y(grad_1_phi, phi, hy, sz, bflag);
    deriv_z(grad_2_phi, phi, hz, sz, bflag);

    nlsm_bcs(chi_rhs, chi, grad_0_chi, grad_1_chi, grad_2_chi, pmin, pmax,
             1.0, 0.0, sz, bflag);
    nlsm_bcs(phi_rhs, phi, grad_0_phi, grad_1_phi, grad_2_phi, pmin, pmax,
             1.0, 0.0, sz, bflag);
    nlsm::timer::t_bdyc.stop();
  }


nlsm::timer::t_deriv.start();
ko_deriv_x(grad_0_chi, chi, hx, sz, bflag);
ko_deriv_y(grad_1_chi, chi, hy, sz, bflag);
ko_deriv_z(grad_2_chi, chi, hz, sz, bflag);

ko_deriv_x(grad_0_phi, phi, hx, sz, bflag);
ko_deriv_y(grad_1_phi, phi, hy, sz, bflag);
ko_deriv_z(grad_2_phi, phi, hz, sz, bflag);
nlsm::timer::t_deriv.stop();

nlsm::timer::t_rhs.start();

  const  double sigma = KO_DISS_SIGMA;


  for (unsigned int k = 3; k < nz-3; k++) {
    for (unsigned int j = 3; j < ny-3; j++) {
      for (unsigned int i = 3; i < nx-3; i++) {
        pp = i + nx*(j + ny*k);

        chi_rhs[pp]  += sigma * (grad_0_chi[pp] + grad_1_chi[pp] + grad_2_chi[pp]);
        phi_rhs[pp]  += sigma * (grad_0_phi[pp] + grad_1_phi[pp] + grad_2_phi[pp]);

      }
    }
  }

  nlsm::timer::t_rhs.stop();



nlsm::timer::t_deriv.start();

delete [] grad2_0_0_chi;
delete [] grad2_1_1_chi;
delete [] grad2_2_2_chi;

delete [] grad_0_chi;
delete [] grad_1_chi;
delete [] grad_2_chi;

delete [] grad_0_phi;
delete [] grad_1_phi;
delete [] grad_2_phi;

nlsm::timer::t_deriv.stop();

#if 0
  for (unsigned int m = 0; m < 24; m++) {
    std::cout<<"  || dtu("<<m<<")|| = "<<normLInfty(unzipVarsRHS[m] + offset, n)<<std::endl;
  }
#endif



}

/*----------------------------------------------------------------------;
 *
 *
 *
 *----------------------------------------------------------------------*/
void nlsm_bcs(double *f_rhs, const double *f,
              const double *dxf, const double *dyf, const double *dzf,
              const double *pmin, const double *pmax,
              const double f_falloff, const double f_asymptotic,
              const unsigned int *sz, const unsigned int &bflag)
{

  const unsigned int nx = sz[0];
  const unsigned int ny = sz[1];
  const unsigned int nz = sz[2];

  double hx = (pmax[0] - pmin[0]) / (nx - 1);
  double hy = (pmax[1] - pmin[1]) / (ny - 1);
  double hz = (pmax[2] - pmin[2]) / (nz - 1);

  unsigned int ib = 3;
  unsigned int jb = 3;
  unsigned int kb = 3;
  unsigned int ie = sz[0]-3;
  unsigned int je = sz[1]-3;
  unsigned int ke = sz[2]-3;

  double x,y,z;
  unsigned int pp;
  double inv_r;

  if (bflag & (1u<<OCT_DIR_LEFT)) {
    double x = pmin[0] + ib*hx;
    for (unsigned int k = kb; k < ke; k++) {
       z = pmin[2] + k*hz;
      for (unsigned int j = jb; j < je; j++) {
         y = pmin[1] + j*hy;
         pp = IDX(ib,j,k);
         inv_r = 1.0 / sqrt(x*x + y*y + z*z);


        f_rhs[pp] = -  inv_r * (
                         x * dxf[pp]
                       + y * dyf[pp]
                       + z * dzf[pp]
                       + f_falloff * (   f[pp] - f_asymptotic ) );

      }
    }
  }

  if (bflag & (1u<<OCT_DIR_RIGHT)) {
     x = pmin[0] + (ie-1)*hx;
    for (unsigned int k = kb; k < ke; k++) {
       z = pmin[2] + k*hz;
      for (unsigned int j = jb; j < je; j++) {
         y = pmin[1] + j*hy;
         pp = IDX((ie-1),j,k);
         inv_r = 1.0 / sqrt(x*x + y*y + z*z);

        f_rhs[pp] = -  inv_r * (
                         x * dxf[pp]
                       + y * dyf[pp]
                       + z * dzf[pp]
                       + f_falloff * (   f[pp] - f_asymptotic ) );

      }
    }
  }

  if (bflag & (1u<<OCT_DIR_DOWN)) {
     y = pmin[1] + jb*hy;
    for (unsigned int k = kb; k < ke; k++) {
       z = pmin[2] + k*hz;
      for (unsigned int i = ib; i < ie; i++) {
         x = pmin[0] + i*hx;
         inv_r = 1.0 / sqrt(x*x + y*y + z*z);
         pp = IDX(i,jb,k);

        f_rhs[pp] = -  inv_r * (
                         x * dxf[pp]
                       + y * dyf[pp]
                       + z * dzf[pp]
                       + f_falloff * (   f[pp] - f_asymptotic ) );

      }
    }
  }

  if (bflag & (1u<<OCT_DIR_UP)) {
     y = pmin[1] + (je-1)*hy;
    for (unsigned int k = kb; k < ke; k++) {
       z = pmin[2] + k*hz;
      for (unsigned int i = ib; i < ie; i++) {
         x = pmin[0] + i*hx;
         inv_r = 1.0 / sqrt(x*x + y*y + z*z);
         pp = IDX(i,(je-1),k);

        f_rhs[pp] = -  inv_r * (
                         x * dxf[pp]
                       + y * dyf[pp]
                       + z * dzf[pp]
                       + f_falloff * (   f[pp] - f_asymptotic ) );

      }
    }
  }

  if (bflag & (1u<<OCT_DIR_BACK)) {
     z = pmin[2] + kb*hz;
    for (unsigned int j = jb; j < je; j++) {
       y = pmin[1] + j*hy;
      for (unsigned int i = ib; i < ie; i++) {
         x = pmin[0] + i*hx;
         inv_r = 1.0 / sqrt(x*x + y*y + z*z);
         pp = IDX(i,j,kb);

        f_rhs[pp] = - inv_r * (
                         x * dxf[pp]
                       + y * dyf[pp]
                       + z * dzf[pp]
                       + f_falloff * (   f[pp] - f_asymptotic ) );

      }
    }
  }

  if (bflag & (1u<<OCT_DIR_FRONT)) {
    z = pmin[2] + (ke-1)*hz;
    for (unsigned int j = jb; j < je; j++) {
      y = pmin[1] + j*hy;
      for (unsigned int i = ib; i < ie; i++) {
        x = pmin[0] + i*hx;
        inv_r = 1.0 / sqrt(x*x + y*y + z*z);
        pp = IDX(i,j,(ke-1));

        f_rhs[pp] = - inv_r * (
                         x * dxf[pp]
                       + y * dyf[pp]
                       + z * dzf[pp]
                       + f_falloff * (   f[pp] - f_asymptotic ) );

      }
    }
  }

}

/*----------------------------------------------------------------------;
 *
 *
 *
 *----------------------------------------------------------------------*/
