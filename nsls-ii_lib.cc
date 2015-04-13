/* NSLS-II specific library

   J. Bengtsson  NSLS-II, BNL  2004 -        

   T. Shaftan, I. Pinayev, Y. Luo, C. Montag, B. Nash

*/
/* Current revision $Revision: 1.13 $
 On branch $Name:  $
 Latest change by $Author: zhang $
*/

#include "tracy_lib.h"

// global params

bool    DA_bare       = false,  // dynamic aperture
        freq_map      = false,  // include frequency map
        tune_shift    = false;  // to calculate tune shift with amplitude and energy
int     n_aper        = 15,     // no of dynamical aperture points
        n_track       = 512,    // no of turns for tracking
        n_orbit       = 3,      // number of integration for orbit correction
        n_scale       = 1,      // number to scale the random orbit error
	n_coupl       = 0;      // number of iterations for coupling correction

bool    bba           = false;   // beam based alignment
int     n_lin         =  3,
        SQ_per_scell  =  2,
        BPM_per_scell = 12,
        HCM_per_scell = 12,
        VCM_per_scell = 12;
double  kick          = 0.01e-3; // 0.01 mrad kick for trims
int     n_stat        = 1;       // number of statistics;when set the random error;

double  VDweight      = 1e3,     // weight for vertical dispersion
        HVweight      = 1e0,     // weight for coupling Htrim vertical BPM
        VHweight      = 1e0;     // weight for coupling Vtrim horizontal BPM


double  delta_DA_  = 5.0e-2; // Parameters for dynamic aperture

// Parameters for frequency map
// Note NTURN is set to 10000 (2*NTURN for diffusion)) in "naffutils.h".
int     n_x = 50, n_y = 30, n_dp = 25, n_tr = 2064;
double  x_max_FMA = 20e-3, y_max_FMA = 6e-3, delta_FMA = 3e-2;
//double  x_max_FMA = 20e-3, y_max_FMA = 3e-3, delta_FMA = 3e-2;

const int  max_corr = 100;
const int  max_bpm  = 200;

int     h_corr[max_corr], v_corr[max_corr], bpm_loc[max_bpm]; //non-global varible

int     Fnum_Cart, n_iter_Cart;

double  u_Touschek;  // argument for Touschek D(ksi)
double  chi_m;       // argument for IBS D(ksi)

char    in_dir[max_str]          = "";

// Computation result files
const char  beam_envelope_file[] = "beam_envelope.out";

// Lattice error and correction files
const char  CodCorLatFileName[]  = "codcorlat.out";

const char  SkewMatFileName[]    = "skewmat.out";
const char  eta_y_FileName[]     = "eta_y.out";
const char  deta_y_FileName[]    = "deta_y.out";
 

const int  max_elem = Cell_nLocMax;

const int  N_Fam_max = 15;

// Weights for ID correction
const double  scl_nu = 1e2, scl_dbeta = 1e0, scl_dnu = 0.1, ID_step = 0.7;

char     ae_file[max_str], fe_file[max_str], ap_file[max_str];
int      N_BPM, N_HCOR, N_VCOR, N_SKEW, N_COUPLE;
int      N_calls, N_steps, N_Fam, Q_Fam[N_Fam_max];  // ID correction params
int      n_sext, sexts[max_elem];
double   beta0_[max_elem][2], nu0s[max_elem][2], nu0_[2], b2[N_Fam_max];
double   **SkewRespMat, *VertCouple, *SkewStrengthCorr, *eta_y;
double   *b, *w, **V, **U;
double   disp_wave_y;
Vector2  beta_ref;

char lat_FileName[max_str];

// ID_corr global variables

const int  n_b2_max    = 500;  // max no of quad corrector families
const int  n_b3_max    = 1000; // max no of sextupoles
const int  max_ID_Fams = 25;   // max no of ID families

int           Nsext, Nquad, Nconstr, NconstrO, quad_prms[n_b2_max], id_loc;
int           n_ID_Fams, ID_Fams[max_ID_Fams];
double        Ss[n_b3_max], Sq[n_b2_max], sb[2][n_b3_max], sNu[2][n_b3_max];
double        qb[2][n_b2_max], qb0[2][n_b2_max], sNu0[2][n_b3_max];
double        qNu0[2][n_b2_max], qNu[2][n_b2_max], IDb[2], IDNu[2];
double        Nu_X, Nu_Y, Nu_X0, Nu_Y0;
double        **A1, *Xsext, *Xsext0, *b2Ls_, *w1, **U1, **V1;
double        *Xoct, *b4s, **Aoct;
Vector2       dnu0, nu_0;

ss_vect<tps>  map;
MNF_struct    MNF;
 

// conversion

void lwr_case(char str[])
{
  int  k;

  for (k = 0; k < (int)strlen(str); k++)
    str[k] = tolower(str[k]);
}


void upr_case(char str[])
{
  int  k;

  for (k = 0; k < (int)strlen(str); k++)
    str[k] = toupper(str[k]);
}


// only supported by Redhat
#if 0
// generate backtrace
void prt_trace (void)
{
  const int  max_entries = 20;

  void    *array[max_entries];
  size_t  size;
  char    **strings;
  size_t  i;
     
  size = backtrace(array, max_entries);
  strings = backtrace_symbols(array, size);
     
  printf("prt_trace: obtained %zd stack frames\n", size);
     
  for (i = 0; i < size; i++)
    printf ("%s\n", strings[i]);
     
  free (strings);
}
#endif


// file I/O

// C++

void file_rd(ifstream &inf, const char file_name[])
{

  inf.open(file_name, ios::in);
  if (!inf.is_open()) {
    printf("File not found: %s\n", file_name);
    exit_(-1);
  }
}


void file_wr(ofstream &outf, const char file_name[])
{

  outf.open(file_name, ios::out);
  if (!outf.is_open()) {
    printf("Could not create file: %s\n", file_name);
    exit_(-1);
  }
}


// C
/***********************************************
FILE* file_read(const char file_name[])

   Purpose:
      Open a file, and return the file ID
************************************************/
FILE* file_read(const char file_name[])
{
  FILE      *fp;
  
  fp = fopen(file_name, "r");
  if (fp == NULL) {
    printf("File not found: %s\n", file_name);
    exit_(-1);
  } else
    return(fp);
    // avoid compiler warning
    return NULL;
}


FILE* file_write(const char file_name[])
{
  FILE      *fp;
  
  fp = fopen(file_name, "w");
  if (fp == NULL) {
    printf("Could not create file: %s\n", file_name);
    exit_(-1);
    // avoid compiler warning
    return NULL;
  } else
    return(fp);
}


void chk_cod(const bool cod, const char *proc_name)
{

  if (!cod) {
    printf("%s: closed orbit not found\n", proc_name);
    exit_(1);
  }
}


void no_sxt(void)
{
  int       k;

  cout << endl;
  cout << "zeroing sextupoles" << endl;
  for (k = 0; k <= globval.Cell_nLoc; k++)
    if ((Cell[k].Elem.Pkind == Mpole) && (Cell[k].Elem.M->Porder >= Sext))
      SetKpar(Cell[k].Fnum, Cell[k].Knum, Sext, 0.0);
}


void get_map(void)
{
  long int  lastpos;


  getcod(0.0, lastpos);

  map.identity(); map += globval.CODvect;
  Cell_Pass(0, globval.Cell_nLoc, map, lastpos);
  map -= globval.CODvect;
}


#if ORDER > 1

tps get_h(void)
{
  ss_vect<tps>  map1, R;

  // Parallel transport nonlinear kick to start of lattice,
  // assumes left to right evaluation.

  if (true)
    // Dragt-Finn factorization
    return LieFact_DF(Inv(MNF.A0*MNF.A1)*map*MNF.A0*MNF.A1, R)*R;
  else {
    // single Lie exponent
    danot_(1); map1 = map; danot_(no_tps);
    return LieFact(Inv(MNF.A0*MNF.A1)*map*Inv(map1)*MNF.A0*MNF.A1);
  }
}

#endif

void get_m2(const ss_vect<tps> &ps, tps m2[])
{
  int  i, j, k;

  k = 0;
  for (i = 0; i < 2*nd_tps; i++)
    for (j = i; j < 2*nd_tps; j++) {
      k++; m2[k-1] = ps[i]*ps[j];
    }
}


ss_vect<tps> get_S(const int n_DOF)
{
  int           j;
  ss_vect<tps>  S;

  S.zero();
  for (j = 0; j < n_DOF; j++) {
    S[2*j] = tps(0.0, 2*j+2); S[2*j+1] = -tps(0.0, 2*j+1);
  }

  return S;
}


ss_vect<tps> tp_S(const int n_DOF, const ss_vect<tps> &A)
{
  int           j, jj[ss_dim];
  ss_vect<tps>  S;

  for (j = 1; j <= ss_dim; j++)
    jj[j-1] = (j <= 2*n_DOF)? 1 : 0;

  S = get_S(n_DOF);

  return -S*PInv(A, jj)*S;
}


void get_dnu(const ss_vect<tps> &A, double dnu[])
{
  int  k;

  for (k = 0; k <= 1; k++) {
    dnu[k] = atan2(A[2*k][2*k+1], A[2*k][2*k])/(2.0*M_PI);
    if (dnu[k] < 0.0) dnu[k] += 1.0;
  }
}


ss_vect<tps> get_A_CS(const ss_vect<tps> &A, double dnu[])
{
  int           k;
  double        c, s;
  ss_vect<tps>  Id, R;

  Id.identity(); R.identity(); get_dnu(A, dnu);
  for (k = 0; k <= 1; k++) {

    c = cos(2.0*M_PI*dnu[k]); s = sin(2.0*M_PI*dnu[k]);
    R[2*k] = c*Id[2*k] - s*Id[2*k+1]; R[2*k+1] = s*Id[2*k] + c*Id[2*k+1];
  }

  return A*R;
}


void get_twoJ(const int n_DOF, const ss_vect<double> &ps,
	      const ss_vect<tps> &A, double twoJ[])
{
  int              j;
  iVector          jj;
  ss_vect<double>  z;

  for (j = 0; j < nv_tps; j++)
    jj[j] = (j < 2*n_DOF)? 1 : 0;

  z = (PInv(A, jj)*ps).cst();

  for (j = 0; j < n_DOF; j++)
    twoJ[j] = sqr(z[2*j]) + sqr(z[2*j+1]);
}


double get_curly_H(const double alpha_x, const double beta_x,
		   const double eta_x, const double etap_x)
{
  double  curly_H, gamma_x;

  gamma_x = (1.0+pow(alpha_x, 2))/beta_x;

  curly_H = gamma_x*sqr(eta_x) + 2.0*alpha_x*eta_x*etap_x + beta_x*sqr(etap_x);

  return curly_H;
}


double get_eps_x(void)
{
  bool             cav, emit;
  long int         lastpos;
  double           eps_x;
  ss_vect<tps>     A;

  /* Note:

        T
       M  J M = J,

        -1       T           |  0  I |        T   | beta   -alpha |
       A   = -J A  J,    J = |       |,    A A  = |               |
                             | -I  0 |            | -alpha  gamma |

     Transform to Floquet Space:

        -1           T
       A   eta = -J A  J eta,

               -1      T  -1                T    T
       H~ = ( A   eta )  A   eta = ( J eta )  A A  ( J eta )

  */

  cav = globval.Cavity_on; emit = globval.emittance;

  globval.Cavity_on = false; globval.emittance = false;

  Ring_GetTwiss(false, 0.0);

  putlinmat(6, globval.Ascr, A); A += globval.CODvect;

  globval.emittance = true;

  Cell_Pass(0, globval.Cell_nLoc, A, lastpos);

  eps_x = 1470.0*pow(globval.Energy, 2)*I5/(I2-I4);

  printf("\n");
  printf("eps_x = %5.3f nm.rad\n", eps_x);
  printf("J_x   = %5.3f, J_z = %5.3f\n", 1.0-I4/I2, 2.0+I4/I2);

  globval.Cavity_on = cav; globval.emittance = emit;

  return eps_x;
}

/****************************************************************************/
/* void GetEmittance(const int Fnum, const bool prt)

   Purpose:
       get the emittance

   Input:
      Fnum  family index
      prt   bool flag, true = print the calcuate information
   Output:
       none

   Return:
       emittance
       
   Global variables:
       

   specific functions:
       

   Comments:
       

****************************************************************************/
void GetEmittance(const int Fnum, const bool prt)
{
  bool          emit, rad, cav, path;
  int           i, j, h_RF;
  long int      lastpos, loc;
  double        C, theta, V_RF, phi0, alpha_z, beta_z, gamma_z;
  double        sigma_s, sigma_delta;
  Vector3       nu;
  Matrix        Ascr;
  ss_vect<tps>  Ascr_map;

  // save state
  rad = globval.radiation; emit = globval.emittance;
  cav = globval.Cavity_on; path = globval.pathlength;

  C = Cell[globval.Cell_nLoc].S; /* Circumference of the ring*/

  // damped system
  globval.radiation = true; globval.emittance  = true;
  globval.Cavity_on = true; globval.pathlength = false;

  Ring_GetTwiss(false, 0.0);

  // radiation loss is computed in Cav_Pass

  // Sum_k( alpha_k) = 2 * U_0 / E
//  printf("\n");
//  printf("%24.16e %24.16e\n",
//	 globval.dE,
//	 (globval.alpha_rad[X_]+globval.alpha_rad[Y_]
//	 +globval.alpha_rad[Z_])/2.0);

  globval.U0 = 1e9*globval.dE*globval.Energy;
  V_RF = Cell[Elem_GetPos(Fnum, 1)].Elem.C->Pvolt; //RF voltage
  h_RF = Cell[Elem_GetPos(Fnum, 1)].Elem.C->Ph;  // RF cavity harmonic number
  phi0 = fabs(asin(globval.U0/V_RF));
  globval.delta_RF =
    sqrt(-V_RF*cos(M_PI-phi0)*(2.0-(M_PI-2.0*(M_PI-phi0))
    *tan(M_PI-phi0))/(globval.Alphac*M_PI*h_RF*1e9*globval.Energy));

  // compute diffusion coeffs. for eigenvectors [sigma_xx, sigma_yy, sigma_zz]
  putlinmat(6, globval.Ascr, Ascr_map); Ascr_map += globval.CODvect;

  Cell_Pass(0, globval.Cell_nLoc, Ascr_map, lastpos);

  for (i = 0; i < DOF; i++) {
    // partition numbers
    globval.J[i] = 2.0*(1.0+globval.CODvect[delta_])*globval.alpha_rad[i]
                   /globval.dE;
    // damping times
    globval.tau[i] = -C/(c0*globval.alpha_rad[i]);
    // diffusion coeff. and emittance
    globval.eps[i] = -globval.D_rad[i]/(2.0*globval.alpha_rad[i]);
    // fractional tunes
    nu[i]  = atan2(globval.wi[i*2], globval.wr[i*2])/(2.0*M_PI);
    if (nu[i] < 0.0) nu[i] = 1.0 + nu[i];
  }

  // Note, J_x + J_y + J_z not exactly 4 (1st order perturbations)
//  printf("\n");
//  printf("%24.16e\n", globval.J[X_]+globval.J[Y_]+globval.J[Z_]);
  
  // undamped system
  globval.radiation = false; globval.emittance = false;

  Ring_GetTwiss(false, 0.0);

  /* compute the sigmas arround the lattice:

       sigma = A diag[J_1, J_1, J_2, J_2, J_3, J_3] A^T

  */
  for (i = 0; i < 6; i++) {
    Ascr_map[i] = tps(globval.CODvect[i]);
    for (j = 0; j < 6; j++)
      Ascr_map[i] += globval.Ascr[i][j]*sqrt(globval.eps[j/2])*tps(0.0, j+1);
  }
  for (loc = 0; loc <= globval.Cell_nLoc; loc++) {
    Elem_Pass(loc, Ascr_map);
    // sigma = A x A^tp
    getlinmat(6, Ascr_map, Cell[loc].sigma); TpMat(6, Cell[loc].sigma);
    getlinmat(6, Ascr_map, Ascr); MulLMat(6, Ascr, Cell[loc].sigma);
  }

  theta = atan2(2e0*Cell[0].sigma[x_][y_],
	  (Cell[0].sigma[x_][x_]-Cell[0].sigma[y_][y_]))/2e0;

  // longitudinal alpha and beta
  alpha_z =
    -globval.Ascr[ct_][ct_]*globval.Ascr[delta_][ct_]
    - globval.Ascr[ct_][delta_]*globval.Ascr[delta_][delta_];
  beta_z = sqr(globval.Ascr[ct_][ct_]) + sqr(globval.Ascr[ct_][delta_]);
  gamma_z = (1.0+sqr(alpha_z))/beta_z;

  // bunch size
  sigma_s = sqrt(beta_z*globval.eps[Z_]);
  sigma_delta = sqrt(gamma_z*globval.eps[Z_]);

  if (prt) {
    printf("\n");
    printf("Emittance:\n");
    printf("\n");
    printf("Energy loss per turn [keV]:     "
	   "U0          = %3.1f\n",
	   1e-3*globval.U0);
    printf("Synchronous phase [deg]:        "
	   "phi0        = 180 - %4.2f\n",
	   phi0*180.0/M_PI);
    printf("RF bucket height [%%]:           "
	   "delta_RF    = %4.2f\n", 1e2*globval.delta_RF);
    printf("\n");
    printf("Equilibrium emittance [m.rad]:  "
	   "eps_x       = %9.3e, eps_y  = %9.3e, eps_z  = %9.3e\n",
            globval.eps[X_], globval.eps[Y_], globval.eps[Z_]);
    printf("Bunch length [mm]:              "
	   "sigma_s     = %5.3f\n", 1e3*sigma_s);
    printf("Momentum spread:                "
	   "sigma_delta = %9.3e\n", sigma_delta);
    printf("Partition numbers:              "
	   "J_x         = %5.3f,     J_y    = %5.3f,     J_z    = %5.3f\n",
            globval.J[X_], globval.J[Y_], globval.J[Z_]);
    printf("Damping times [msec]:           "
	   "tau_x       = %3.1f,      tau_y  = %3.1f,      tau_z  = %3.1f\n",
	   1e3*globval.tau[X_], 1e3*globval.tau[Y_], 1e3*globval.tau[Z_]);
    printf("\n");
    printf("alphac:                         "
	   "alphac      = %8.4e\n", globval.Alphac); 
    printf("\n");
    printf("Fractional tunes:               "
	   "nu_x        = %7.5f, nu_y   = %7.5f, nu_z   = %7.5f\n",
	   nu[X_], nu[Y_], nu[Z_]);
    printf("                                "
	   "1-nu_x      = %7.5f, 1-nu_y = %7.5f, 1-nu_z = %7.5f\n",
	   1e0-nu[X_], 1e0-nu[Y_], 1e0-nu[Z_]);
    printf("\n");
    printf("sigmas:                         "
	   "sigma_x     = %5.1f microns, sigma_px    = %5.1f urad\n",
	   1e6*sqrt(Cell[0].sigma[x_][x_]), 1e6*sqrt(Cell[0].sigma[px_][px_]));
    printf("                                "
	   "sigma_y     = %5.1f microns, sigma_py    = %5.1f urad\n",
	   1e6*sqrt(Cell[0].sigma[y_][y_]), 1e6*sqrt(Cell[0].sigma[py_][py_]));
    printf("                                "
	   "sigma_s     = %5.2f mm,      sigma_delta = %8.2e\n",
	   1e3*sqrt(Cell[0].sigma[ct_][ct_]),
	   sqrt(Cell[0].sigma[delta_][delta_]));

    printf("\n");
    printf("Beam ellipse twist [rad]:       tw = %5.3f\n", theta);
    printf("                   [deg]:       tw = %5.3f\n", theta*180.0/M_PI);
  }

  // restore state
  globval.radiation = rad; globval.emittance  = emit;
  globval.Cavity_on = cav; globval.pathlength = path;
}


// output

void prt_lat(const char *fname, const int Fnum, const bool all)
{
  long int      i = 0;
  double        I2, I5, code = 0.0;
  FILE          *outf;

  outf = file_write(fname);
  fprintf(outf, "#        name           s   code"
	        "  alphax  betax   nux   etax   etapx");
  fprintf(outf, "  alphay  betay   nuy   etay   etapy    I5\n");
  fprintf(outf, "#                      [m]"
	        "                 [m]           [m]");
  fprintf(outf, "                   [m]           [m]\n");
  fprintf(outf, "#\n");

  I2 = 0.0; I5 = 0.0;
  for (i = 0; i <= globval.Cell_nLoc; i++) {
    if (all || (Cell[i].Fnum == Fnum)) {
      switch (Cell[i].Elem.Pkind) {
      case drift:
	code = 0.0;
	break;
      case Mpole:
	if (Cell[i].Elem.M->Pirho != 0.0)
	  code = 0.5;
	else if (Cell[i].Elem.M->PBpar[Quad+HOMmax] != 0)
	  code = sgn(Cell[i].Elem.M->PBpar[Quad+HOMmax]);
	else if (Cell[i].Elem.M->PBpar[Sext+HOMmax] != 0)
	  code = 1.5*sgn(Cell[i].Elem.M->PBpar[Sext+HOMmax]);
	else if (Cell[i].Fnum == globval.bpm)
	  code = 2.0;
	else
	  code = 0.0;
	break;
      default:
	code = 0.0;
	break;
      }
      fprintf(outf, "%4ld %15s %6.2f %4.1f"
	      " %7.3f %6.3f %6.3f %6.3f %6.3f"
	      " %7.3f %6.3f %6.3f %6.3f %6.3f  %8.2e\n",
	      i, Cell[i].Elem.PName, Cell[i].S, code,
	      Cell[i].Alpha[X_], Cell[i].Beta[X_], Cell[i].Nu[X_],
	      Cell[i].Eta[X_], Cell[i].Etap[X_],
	      Cell[i].Alpha[Y_], Cell[i].Beta[Y_], Cell[i].Nu[Y_],
	      Cell[i].Eta[Y_], Cell[i].Etap[Y_], I5);
    }
  }

//  fprintf(outf, "\n");
//  fprintf(outf, "# emittance: %5.3f nm.rad\n", get_eps_x());

  fclose(outf);
}


void prt_chrom_lat(void)
{
  long int  i;
  double    dbeta_ddelta[Cell_nLocMax][2], detax_ddelta[Cell_nLocMax];
  double    ksi[Cell_nLocMax][2];
  double    code = 0.0;
  FILE      *outf;

  printf("\n");
  printf("prt_chrom_lat: calling Ring_GetTwiss with delta != 0\n");
  Ring_GetTwiss(true, globval.dPcommon);
  for (i = 0; i <= globval.Cell_nLoc; i++) {
    dbeta_ddelta[i][X_] = Cell[i].Beta[X_];
    dbeta_ddelta[i][Y_] = Cell[i].Beta[Y_];
    detax_ddelta[i] = Cell[i].Eta[X_];
  }
  printf("prt_chrom_lat: calling Ring_GetTwiss with delta != 0\n");
  Ring_GetTwiss(true, -globval.dPcommon);
  ksi[0][X_] = 0.0; ksi[0][Y_] = 0.0;
  for (i = 0; i <= globval.Cell_nLoc; i++) {
    dbeta_ddelta[i][X_] -= Cell[i].Beta[X_];
    dbeta_ddelta[i][Y_] -= Cell[i].Beta[Y_];
    detax_ddelta[i] -= Cell[i].Eta[X_];
    dbeta_ddelta[i][X_] /= 2.0*globval.dPcommon;
    dbeta_ddelta[i][Y_] /= 2.0*globval.dPcommon;
    detax_ddelta[i] /= 2.0*globval.dPcommon;
    if (i != 0) {
      ksi[i][X_] = ksi[i-1][X_]; ksi[i][Y_] = ksi[i-1][Y_];
    }
    if (Cell[i].Elem.Pkind == Mpole) {
	ksi[i][X_] -= Cell[i].Elem.M->PBpar[Quad+HOMmax]
                     *Cell[i].Elem.PL*Cell[i].Beta[X_]/(4.0*M_PI);
	ksi[i][Y_] += Cell[i].Elem.M->PBpar[Quad+HOMmax]
                     *Cell[i].Elem.PL*Cell[i].Beta[Y_]/(4.0*M_PI);
    }
  }

  outf = file_write("chromlat.out");
  fprintf(outf, "#     name              s    code"
	        "  bx*ex  sqrt(bx*by)  dbx/dd*ex  bx*dex/dd"
	        "  by*ex  dby/dd*ex by*dex/dd  ksix  ksiy"
	        "  dbx/dd  bx/dd dex/dd\n");
  fprintf(outf, "#                      [m]          [m]"
	        "      [m]          [m]       [m]");
  fprintf(outf, "       [m]      [m]       [m]\n");
  fprintf(outf, "#\n");
  for (i = 0; i <= globval.Cell_nLoc; i++) {
    switch (Cell[i].Elem.Pkind) {
    case drift:
      code = 0.0;
      break;
    case Mpole:
      if (Cell[i].Elem.M->Pirho != 0)
	code = 0.5;
      else if (Cell[i].Elem.M->PBpar[Quad+HOMmax] != 0)
	code = sgn(Cell[i].Elem.M->PBpar[Quad+HOMmax]);
      else if (Cell[i].Elem.M->PBpar[Sext+HOMmax] != 0)
	code = 1.5*sgn(Cell[i].Elem.M->PBpar[Sext+HOMmax]);
      else if (Cell[i].Fnum == globval.bpm)
        code = 2.0;
      else
	code = 0.0;
      break;
    default:
      code = 0.0;
      break;
    }
    fprintf(outf, "%4ld %15s %6.2f %4.1f"
	          "  %6.3f  %8.3f    %8.3f   %8.3f"
	          "   %6.3f %8.3f   %8.3f  %5.2f  %5.2f"
	          "  %6.3f  %6.3f  %6.3f\n",
	    i, Cell[i].Elem.PName, Cell[i].S, code,
	    Cell[i].Beta[X_]*Cell[i].Eta[X_],
	    sqrt(Cell[i].Beta[X_]*Cell[i].Beta[Y_]),
	    dbeta_ddelta[i][X_]*Cell[i].Eta[X_],
	    detax_ddelta[i]*Cell[i].Beta[X_],
	    Cell[i].Beta[Y_]*Cell[i].Eta[X_],
	    dbeta_ddelta[i][Y_]*Cell[i].Eta[X_],
	    detax_ddelta[i]*Cell[i].Beta[Y_],
	    ksi[i][X_], ksi[i][Y_],
	    dbeta_ddelta[i][X_], dbeta_ddelta[i][Y_], detax_ddelta[i]);
  }
  fclose(outf);
}

/****************************************************************************/
/* void prt_cod(const char *file_name, const int Fnum, const bool all)

   Purpose:
       print the location, twiss parameters, displacement errors and close orbit 
       at the the location of Fnum or all lattice elements

   Input:
      file_name    file to save the data
                    code in the file:
		      0   drfit
		      0.5  dipole
		      -1    defocusing quadrupole 
		      1     focusing quadrupole
		      1.5  sextupole
		      2.0  bpm 
      Fnum        family index
      all         true, print the cod at all elements
                  false, print the cod at the family elements
   Output:
       none

   Return:
       none
       
   Global variables:
       

   specific functions:
       

   Comments:
       

****************************************************************************/
void prt_cod(const char *file_name, const int Fnum, const bool all)
{
  long      i;
  double    code = 0.0;
  FILE      *outf;
  long      FORLIM;
  struct    tm *newtime;

  outf = file_write(file_name);

  /* Get time and date */
  newtime = GetTime();

  fprintf(outf,"# TRACY III -- %s -- %s \n",
	  file_name, asctime2(newtime));

  fprintf(outf, "#       name             s  code  betax   nux   betay   nuy"
	  "   xcod   ycod    dSx    dSy   dipx   dipy\n");
  fprintf(outf, "#                       [m]        [m]           [m]       "
	  "   [mm]   [mm]    [mm]   [mm] [mrad]  [mrad]\n");
  fprintf(outf, "#\n");

  FORLIM = globval.Cell_nLoc;
  for (i = 0L; i <= FORLIM; i++) {
    if (all || (Cell[i].Fnum == Fnum)) {
      switch (Cell[i].Elem.Pkind) {
      case drift:
	code = 0.0;
	break;
      case Mpole:
	if (Cell[i].Elem.M->Pirho != 0)
	  code = 0.5;
	else if (Cell[i].Elem.M->PBpar[Quad+HOMmax] != 0)
	  code = sgn(Cell[i].Elem.M->PBpar[Quad+HOMmax]);
	else if (Cell[i].Elem.M->PBpar[Sext+HOMmax] != 0)
	  code = 1.5*sgn(Cell[i].Elem.M->PBpar[Sext+HOMmax]);
        else if (Cell[i].Fnum == globval.bpm)
          code = 2.0;
        else
	  code = 0.0;
	break;
      default:
	code = 0.0;
	break;
      }
      /* COD is in local coordinates */
      fprintf(outf, "%4ld %.*s %6.2f %4.1f %6.3f %6.3f %6.3f %6.3f"
	      " %23.16e %23.16e %6.3f %6.3f %6.3f %6.3f\n",
	      i, SymbolLength, Cell[i].Elem.PName, Cell[i].S, code,
	      Cell[i].Beta[X_], Cell[i].Nu[X_],
	      Cell[i].Beta[Y_], Cell[i].Nu[Y_],  
	      1e3*Cell[i].BeamPos[x_], 1e3*Cell[i].BeamPos[y_],
	      1e3*Cell[i].dS[X_], 1e3*Cell[i].dS[Y_], //displacement error
	      -1e3*Elem_GetKval(Cell[i].Fnum, Cell[i].Knum, Dip),  //H kick angle, for correctors
	      1e3*Elem_GetKval(Cell[i].Fnum, Cell[i].Knum, -Dip)); //kick angle, for correctors
    }
  }
  fclose(outf);
}


void prt_beampos(const char *file_name)
{
  int       k;
  ofstream  outf;

  file_wr(outf, file_name);

  outf << "# k  s  name x   px    y   py   delta ct" << endl;
  outf << "#" << endl;

  for (k = 0; k <= globval.Cell_nLoc; k++)
    outf << scientific << setprecision(5)
	 << setw(5) << k << setw(11) << Cell[k].Elem.PName
	 << setw(13) << Cell[k].BeamPos << endl;

  outf.close();
}


/****************************************************************
void CheckAlignTol(const char *OutputFile)

  Purpose:
    check aligment errors of individual magnets on giders
    the dT and roll angle are all printed out
****************************************************************/
// misalignments
void CheckAlignTol(const char *OutputFile)
  // check aligment errors of individual magnets on giders
  // the dT and roll angle are all printed out
{
  int i,j;
  int  n_girders;
  int  gs_Fnum, ge_Fnum;
  int  gs_nKid, ge_nKid;
  int  dip_Fnum,dip_nKid;
  int  loc, loc_gs, loc_ge;
  char * name;
  double s;
  double PdSsys[2], PdSrms[2], PdSrnd[2], dS[2], dT[2];
  fstream fout;
  
  gs_Fnum = globval.gs;   gs_nKid = GetnKid(gs_Fnum);
  ge_Fnum = globval.ge;   ge_nKid = GetnKid(ge_Fnum);
  if (gs_nKid == ge_nKid)
    n_girders= gs_nKid;
  else {
    cout << " The numbers of GS and GE not same. " << endl;
    exit (1);
  }

  fout.open(OutputFile,ios::out);
  if(!fout) {
    cout << "error in opening the file  " << endl;
    exit_(0);
  }
  
  fout << "Girders, Quads, Sexts:  " << endl; 
  for (i = 1; i <= n_girders; i++){
    fout << i << ":" << endl;
    loc_gs = Elem_GetPos(gs_Fnum, i); loc_ge = Elem_GetPos(ge_Fnum, i);
    
    loc = loc_gs;
    PdSsys[X_] = Cell[loc].Elem.M->PdSsys[X_];
    PdSsys[Y_] = Cell[loc].Elem.M->PdSsys[Y_];
    PdSrms[X_] = Cell[loc].Elem.M->PdSrms[X_];
    PdSrms[Y_] = Cell[loc].Elem.M->PdSrms[Y_];
    PdSrnd[X_] = Cell[loc].Elem.M->PdSrnd[X_];
    PdSrnd[Y_] = Cell[loc].Elem.M->PdSrnd[Y_];
    dS[X_] = Cell[loc].dS[X_]; dS[Y_] = Cell[loc].dS[Y_];
    dT[0] = Cell[loc].dT[0]; dT[1] = Cell[loc].dT[1];
    s = Cell[loc].S; name = Cell[loc].Elem.PName;
    fout << "  " << name << "  " << loc << "   " << s
	 << "  " <<  PdSsys[X_] << "  " <<  PdSsys[Y_]
	 << "   " << PdSrms[X_] << "  " <<  PdSrms[Y_]
	 << "   " << PdSrnd[X_] << "  " <<  PdSrnd[Y_]
         << "   " << Cell[loc].Elem.M->PdTrms << "  "
	 << Cell[loc].Elem.M->PdTrnd << "   " << dS[X_]     << "  " <<  dS[Y_]
	 << "   " << atan2( dT[1], dT[0] )  << endl;
    
    for (j = loc_gs+1; j < loc_ge; j++) {
      if ((Cell[j].Elem.Pkind == Mpole) &&
	  (Cell[j].Elem.M->n_design >= Quad || 
	   Cell[j].Elem.M->n_design >= Sext)) {
        loc = j;
	PdSsys[X_] = Cell[loc].Elem.M->PdSsys[X_];
	PdSsys[Y_] = Cell[loc].Elem.M->PdSsys[Y_];
	PdSrms[X_] = Cell[loc].Elem.M->PdSrms[X_];
	PdSrms[Y_] = Cell[loc].Elem.M->PdSrms[Y_];
	PdSrnd[X_] = Cell[loc].Elem.M->PdSrnd[X_];
	PdSrnd[Y_] = Cell[loc].Elem.M->PdSrnd[Y_];
	dS[X_] = Cell[loc].dS[X_]; dS[Y_] = Cell[loc].dS[Y_];
	dT[0] = Cell[loc].dT[0];   dT[1] = Cell[loc].dT[1];
	s = Cell[loc].S; name=Cell[loc].Elem.PName;
	fout << "  " << name << "  " << loc << "   " << s
	     << "  " <<  PdSsys[X_] << "  " <<  PdSsys[Y_]
	     << "   " << PdSrms[X_] << "  " <<  PdSrms[Y_]
	     << "   " << PdSrnd[X_] << "  " <<  PdSrnd[Y_]
	     << "   " << Cell[loc].Elem.M->PdTrms << "  "
	     << Cell[loc].Elem.M->PdTrnd
	     << "   " << dS[X_] << "  " <<  dS[Y_]
	     << "   " << atan2( dT[1], dT[0] )  << endl;
      }
    }
    
    loc = loc_ge;
    PdSsys[X_] = Cell[loc].Elem.M->PdSsys[X_];
    PdSsys[Y_] = Cell[loc].Elem.M->PdSsys[Y_];
    PdSrms[X_] = Cell[loc].Elem.M->PdSrms[X_];
    PdSrms[Y_] = Cell[loc].Elem.M->PdSrms[Y_];
    PdSrnd[X_] = Cell[loc].Elem.M->PdSrnd[X_];
    PdSrnd[Y_] = Cell[loc].Elem.M->PdSrnd[Y_];
    dS[X_] = Cell[loc].dS[X_]; dS[Y_] = Cell[loc].dS[Y_];
    dT[0] = Cell[loc].dT[0]; dT[1] = Cell[loc].dT[1];
    s=Cell[loc].S; name=Cell[loc].Elem.PName;
    fout << "  " << name << "  " << loc << "   " << s
	 << "  " <<  PdSsys[X_] << "  " <<  PdSsys[Y_]
	 << "   " << PdSrms[X_] << "  " <<  PdSrms[Y_]
	 << "   " << PdSrnd[X_] << "  " <<  PdSrnd[Y_]
         << "   " << Cell[loc].Elem.M->PdTrms
	 << "  " << Cell[loc].Elem.M->PdTrnd
         << "   " << dS[X_]     << "  " <<  dS[Y_]
	 << "   " << atan2( dT[1], dT[0] )  << endl;

  }

  fout << "  " << endl;  
  fout << "Dipoles:  " << endl;
  dip_Fnum = ElemIndex("B1"); dip_nKid = GetnKid(dip_Fnum);
  for (i = 1; i <= dip_nKid; i++){
    loc = Elem_GetPos(dip_Fnum, i);
    PdSsys[X_] = Cell[loc].Elem.M->PdSsys[X_];
    PdSsys[Y_] = Cell[loc].Elem.M->PdSsys[Y_];
    PdSrms[X_] = Cell[loc].Elem.M->PdSrms[X_];
    PdSrms[Y_] = Cell[loc].Elem.M->PdSrms[Y_];
    PdSrnd[X_] = Cell[loc].Elem.M->PdSrnd[X_];
    PdSrnd[Y_] = Cell[loc].Elem.M->PdSrnd[Y_];
    dS[X_] = Cell[loc].dS[X_]; dS[Y_] = Cell[loc].dS[Y_];
    dT[0] = Cell[loc].dT[0]; dT[1] = Cell[loc].dT[1];
    s = Cell[loc].S; name = Cell[loc].Elem.PName;
    fout << "  " << name << "  " << loc << "   " << s
	 << "  " <<  PdSsys[X_] << "  " <<  PdSsys[Y_]
	 << "   " << PdSrms[X_] << "  " <<  PdSrms[Y_]
	 << "   " << PdSrnd[X_] << "  " <<  PdSrnd[Y_]
	 << "   " << Cell[loc].Elem.M->PdTrms 
	 << "  " << Cell[loc].Elem.M->PdTrnd
	 << "   " << dS[X_]     << "  " <<  dS[Y_]
	 << "   " << atan2( dT[1], dT[0] )  << endl;
  }
  
  fout.close();
} 

/*********************************************************************
void misalign_rms_elem(const int Fnum, const int Knum,
		       const double dx_rms, const double dy_rms,
		       const double dr_rms, const bool new_rnd)
		       
  Purpose:
     Set random misalignment to the element with Fnum and Knum 
  
  Input:   
     Fnum          family number
     Knum          kid number
     dx_rms         rms value of the error in x
     dy_rms         rms value of the error in y
     dr_rms         rms value of the error in rotation around s
     new_rnd        flag to turn on/off using the new random seed
     
**********************************************************************/
void misalign_rms_elem(const int Fnum, const int Knum,
		const double dx_rms, const double dy_rms,
		const double dr_rms, const bool new_rnd)
{
	long int   loc;
	MpoleType  *mp;

	const bool  normal = true;

	loc = Elem_GetPos(Fnum, Knum); mp = Cell[loc].Elem.M;

	mp->PdSrms[X_] = dx_rms; mp->PdSrms[Y_] = dy_rms; mp->PdTrms = dr_rms;
	if (new_rnd) {
		if (normal) {
			mp->PdSrnd[X_] = normranf(); mp->PdSrnd[Y_] = normranf();
			mp->PdTrnd = normranf();
		} else {
			mp->PdSrnd[X_] = ranf(); mp->PdSrnd[Y_] = ranf();
			mp->PdTrnd = ranf();
		}
	}

	Mpole_SetdS(Fnum, Knum); Mpole_SetdT(Fnum, Knum);
}


/*********************************************************************
void misalign_sys_elem(const int Fnum, const int Knum,
		       const double dx_sys, const double dy_sys,
		       const double dr_sys)
		       
  Purpose:
     Set systematic misalignment to the elements in "type" 
  
  Input:   
     Fnum          family number
     Knum          kid number	       
     dx_sys         sys value of the error in x
     dy_sys         sys value of the error in y
     dr_sys         sys value of the error in rotation around s
     
**********************************************************************/
void misalign_sys_elem(const int Fnum, const int Knum,
		       const double dx_sys, const double dy_sys,
		       const double dr_sys)
{
  long int   loc;
  MpoleType  *mp;

  loc = Elem_GetPos(Fnum, Knum); mp = Cell[loc].Elem.M;

  mp->PdSsys[X_] = dx_sys; mp->PdSsys[Y_] = dy_sys; mp->PdTsys = dr_sys; 

  Mpole_SetdS(Fnum, Knum); Mpole_SetdT(Fnum, Knum);
}

/*********************************************************************
void misalign_rms_fam(const int Fnum,
		      const double dx_rms, const double dy_rms,
		      const double dr_rms, const bool new_rnd)
		       
  Purpose:
     Set systematic misalignment to the elements on the girders
  
  Input:   
     Fnum           family number		       
     dx_rms         rms value of the error in x
     dy_rms         rms value of the error in y
     dr_rms         rms value of the error in rotation around s
     new_rnd        flag to generate random number
**********************************************************************/
void misalign_rms_fam(const int Fnum,
		      const double dx_rms, const double dy_rms,
		      const double dr_rms, const bool new_rnd)
{
  int  i;

  for (i = 1; i <= GetnKid(Fnum); i++)
    misalign_rms_elem(Fnum, i, dx_rms, dy_rms, dr_rms, new_rnd);
}

void misalign_sys_fam(const int Fnum,
		      const double dx_sys, const double dy_sys,
		      const double dr_sys)
{
  int  i;

  for (i = 1; i <= GetnKid(Fnum); i++)
    misalign_sys_elem(Fnum, i, dx_sys, dy_sys, dr_sys);
}

/*********************************************************************
void misalign_rms_type(const int type,
		       const double dx_rms, const double dy_rms,
		       const double dr_rms, const bool new_rnd)
		       
  Purpose:
     Set random misalignment to the elements in "type" 
  
  Input:   
     type           element type,has the following value
                       All
		       dipole
		       quad
		       sext
		       bpm
		       girder
		       
     dx_rms         rms value of the error in x
     dy_rms         rms value of the error in y
     dr_rms         rms value of the error in rotation around s
     new_rnd        flag to turn on/off using the new random seed
     
**********************************************************************/
void misalign_rms_type(const int type,
		       const double dx_rms, const double dy_rms,
		       const double dr_rms, const bool new_rnd)
{
  long int   k;

  if ((type >= All) && (type <= HOMmax)) {
    for (k = 1; k <= globval.Cell_nLoc; k++) {
      if ((Cell[k].Elem.Pkind == Mpole) &&
	  ((type == Cell[k].Elem.M->n_design) ||
	  ((type == All) &&
	   ((Cell[k].Fnum != globval.gs) && (Cell[k].Fnum != globval.ge))))) {
	// if all: skip girders
	misalign_rms_elem(Cell[k].Fnum, Cell[k].Knum,
			  dx_rms, dy_rms, dr_rms, new_rnd);
      }
    }
  } else {
    printf("misalign_rms_type: incorrect type %d\n", type); exit_(1);
  }
}

/*********************************************************************
void misalign_sys_type(const int type,
		       const double dx_sys, const double dy_sys,
		       const double dr_sys)
		       
  Purpose:
     Set systematic misalignment to the elements in "type" 
  
  Input:   
     type           element type,has the following value
                       All
		       dipole
		       quad
		       sext
		       bpm
		       girder
		       
     dx_sys         sys value of the error in x
     dy_sys         sys value of the error in y
     dr_sys         sys value of the error in rotation around s
     
**********************************************************************/
void misalign_sys_type(const int type,
		       const double dx_sys, const double dy_sys,
		       const double dr_sys)
{
  long int   k;

  if ((type >= All) && (type <= HOMmax)) {
    for (k = 1; k <= globval.Cell_nLoc; k++) {
      if ((Cell[k].Elem.Pkind == Mpole) &&
	  ((type == Cell[k].Elem.M->n_design) ||
	  ((type == All) &&
	   ((Cell[k].Fnum != globval.gs) && (Cell[k].Fnum != globval.ge))))) {
	// if all: skip girders
	misalign_sys_elem(Cell[k].Fnum, Cell[k].Knum,
			  dx_sys, dy_sys, dr_sys);
      }
    }
  } else {
    printf("misalign_sys_type: incorrect type %d\n", type); exit_(1);
  }
}

/*********************************************************************
void misalign_rms_girders(const int gs, const int ge,
			  const double dx_rms, const double dy_rms,
			  const double dr_rms, const bool new_rnd)
		       
  Purpose:
     Set systematic misalignment to the elements on the girders
  
  Input:   
     gs             girder start marker
     ge             girder end marker		       
     dx_rms         rms value of the error in x
     dy_rms         rms value of the error in y
     dr_rms         rms value of the error in rotation around s
     
**********************************************************************/
void misalign_rms_girders(const int gs, const int ge,
			  const double dx_rms, const double dy_rms,
			  const double dr_rms, const bool new_rnd)
{
  int       i, k, n_girders, n_ge, n_gs;
  long int  loc_gs, loc_ge, j;
  double    s_gs, s_ge, dx_gs[2], dx_ge[2], s;

  n_gs = GetnKid(gs); n_ge = GetnKid(ge);

  if (n_gs == n_ge)
    n_girders = n_gs;
  else {
    cout << "set_girders: no of GS != no of GE" << endl;
    exit (1);
  }
  
  misalign_rms_fam(gs, dx_rms, dy_rms, dr_rms, new_rnd);
  misalign_rms_fam(ge, dx_rms, dy_rms, dr_rms, new_rnd);

  for (i = 1; i <= n_girders; i++) {
    loc_gs = Elem_GetPos(gs, i); loc_ge = Elem_GetPos(ge, i);
    s_gs = Cell[loc_gs].S; s_ge = Cell[loc_ge].S;

    // roll for a rigid boby
    // Note, girders needs to be introduced as gs->ge pairs
    Cell[loc_ge].Elem.M->PdTrnd = Cell[loc_gs].Elem.M->PdTrnd;
    Mpole_SetdT(ge, i);

    for (k = 0; k <= 1; k++) {
      dx_gs[k] = Cell[loc_gs].dS[k]; dx_ge[k] = Cell[loc_ge].dS[k];
    }

    // move elements onto mis-aligned girder
    for (j = loc_gs+1; j < loc_ge; j++) {
      if ((Cell[j].Elem.Pkind == Mpole) || (Cell[j].Fnum == globval.bpm)) {
        s = Cell[j].S;
	for (k = 0; k <= 1; k++)
	  Cell[j].Elem.M->PdSsys[k]
	    = dx_gs[k] + (dx_ge[k]-dx_gs[k])*(s-s_gs)/(s_ge-s_gs);
	Cell[j].Elem.M->PdTsys = 
	  Cell[loc_gs].Elem.M->PdTrms*Cell[loc_gs].Elem.M->PdTrnd;
      }
    }
  }
}

/*********************************************************************
void misalign_sys_girders(const int gs, const int ge,
			  const double dx_sys, const double dy_sys,
			  const double dr_sys)
		       
  Purpose:
     Set systematic misalignment to the elements on the girders
  
  Input:   
     gs             girder start marker
     ge             girder end marker		       
     dx_sys         sys value of the error in x
     dy_sys         sys value of the error in y
     dr_sys         sys value of the error in rotation around s
     
**********************************************************************/
void misalign_sys_girders(const int gs, const int ge,
			  const double dx_sys, const double dy_sys,
			  const double dr_sys)
{
  int       i, k, n_girders, n_ge, n_gs;
  long int  loc_gs, loc_ge, j;
  double    s_gs, s_ge, dx_gs[2], dx_ge[2], s;

  n_gs = GetnKid(gs); n_ge = GetnKid(ge);

  if (n_gs == n_ge)
    n_girders = n_gs;
  else {
    cout << "set_girders: no of GS != no of GE" << endl;
    exit (1);
  }
  
  misalign_sys_fam(gs, dx_sys, dy_sys, dr_sys);
  misalign_sys_fam(ge, dx_sys, dy_sys, dr_sys);

  for (i = 1; i <= n_girders; i++) {
    loc_gs = Elem_GetPos(gs, i); loc_ge = Elem_GetPos(ge, i);
    s_gs = Cell[loc_gs].S; s_ge = Cell[loc_ge].S;

    // roll for a rigid boby
    // Note, girders needs to be introduced as gs->ge pairs
    Cell[loc_ge].Elem.M->PdTrnd = Cell[loc_gs].Elem.M->PdTrnd;
    Mpole_SetdT(ge, i);

    for (k = 0; k <= 1; k++) {
      dx_gs[k] = Cell[loc_gs].dS[k]; dx_ge[k] = Cell[loc_ge].dS[k];
    }

    // move elements onto mis-aligned girder
    for (j = loc_gs+1; j < loc_ge; j++) {
      if ((Cell[j].Elem.Pkind == Mpole) || (Cell[j].Fnum == globval.bpm)) {
        s = Cell[j].S;
	for (k = 0; k <= 1; k++)
	  Cell[j].Elem.M->PdSsys[k]
	    = dx_gs[k] + (dx_ge[k]-dx_gs[k])*(s-s_gs)/(s_ge-s_gs);
	Cell[j].Elem.M->PdTsys = 
	  Cell[loc_gs].Elem.M->PdTrms*Cell[loc_gs].Elem.M->PdTrnd;
      }
    }
  }
}

/***************************************************************************
void LoadAlignTol(const char *AlignFile, const bool Scale_it,
		  const double Scale, const bool new_rnd, const int k)

  Purpose:
        Load misalignment error	from an external file, then set the error
	to the corresponding elements.
 
  Input:
      AlignFile    file from which the misalignment error is readed  	  		  
      Scale_it     flag to turn on/off the scale of the error
      Scale        value to scale the error
      new_rnd      use new random number or not
      k   
****************************************************************************/
void LoadAlignTol(const char *AlignFile, const bool Scale_it,
		const double Scale, const bool new_rnd, const int k)
{
	char    line[max_str], Name[max_str],  type[max_str];
	int     Fnum, seed_val, ki;
	double  dx, dy, dr;  // x and y misalignments [m] and roll error [rad]
	double  dr_deg, anr, bnr;
	bool    rms = false, set_rnd;
	FILE    *fp;
	MpoleType  *mp;

	fp = file_read(AlignFile);

	printf("\n");
	if (new_rnd)
		printf("set alignment errors\n");
	else
		printf("scale alignment errors: %4.2f\n", Scale);

	set_rnd = false;
	while (fgets(line, max_str, fp) != NULL) {
		//check for whether to set seed
		if ((strstr(line, "#") == NULL) && (strcmp(line, "\r\n") != 0)) {
			sscanf(line, "%s", Name);
			if (strcmp("seed", Name) == 0) {
				set_rnd = true;
				sscanf(line, "%*s %d", &seed_val);
				seed_val += 2*k;
				printf("setting random seed to %d\n", seed_val);
				iniranf(seed_val);
			} else {
				sscanf(line,"%*s %s %lf %lf %lf", type, &dx, &dy, &dr);
				dr_deg = dr*180.0/M_PI;

				if (strcmp(type, "rms") == 0){
					rms = true;
					printf("<rms> ");
				}
				else if (strcmp(type, "sys") == 0){
					rms = false;
					printf("<sys> ");
				}
				else {
					printf("LoadAlignTol: element %s:  need to specify rms or sys\n",
							Name);
					exit_(1);
				}

				if (rms && !set_rnd) {
					printf("LoadAlignTol: seed not defined\n");
					exit_(1);
				}

				if (Scale_it) {
					dx *= Scale; dy *= Scale; dr *= Scale;
				}

				if (strcmp("all", Name) == 0) {
					printf("misaligning all:         dx = %e, dy = %e, dr = %e\n",
							dx, dy, dr);
					if(rms)
						misalign_rms_type(All, dx, dy, dr_deg, new_rnd);
					else
						misalign_sys_type(All, dx, dy, dr_deg);
				} else if (strcmp("girder", Name) == 0) {
					printf("misaligning girders:     dx = %e, dy = %e, dr = %e\n",
							dx, dy, dr);
					if (rms)
						misalign_rms_girders(globval.gs, globval.ge, dx, dy, dr_deg,
								new_rnd);
					else
						misalign_sys_girders(globval.gs, globval.ge, dx, dy, dr_deg);
				} else if (strcmp("dipole", Name) == 0) {
					printf("misaligning dipoles:     dx = %e, dy = %e, dr = %e\n",
							dx, dy, dr);
					if (rms){
						// Inserido por Fernando em 30/11/2011
						for(ki = 1; ki <= globval.Cell_nLoc; ki++)
							if ((Cell[ki].Elem.Pkind == Mpole) && (Cell[ki].Elem.M->n_design == Dip)) {
								mp = Cell[ki].Elem.M;
								bnr = (mp->PBrms[HOMmax+Dip])/(mp->Pirho);
								anr = (mp->PBrms[HOMmax-Dip])/(mp->Pirho);
								break;
							}

						set_bnr_rms_type(Dip, Dip, bnr, anr, new_rnd); //introduzido por Fernando em 30/11/2011
						misalign_rms_type(Dip, dx, dy, dr_deg, new_rnd);
					} else
						misalign_sys_type(Dip, dx, dy, dr_deg);
				} else if (strcmp("quad", Name) == 0) {
					printf("misaligning quadrupoles: dx = %e, dy = %e, dr = %e\n",
							dx, dy, dr);
					if (rms) {
						// Inserido por Fernando em 30/11/2011
						for(ki = 1; ki <= globval.Cell_nLoc; ki++)
							if ((Cell[ki].Elem.Pkind == Mpole) && (Cell[ki].Elem.M->n_design == Quad)) {
								mp = Cell[ki].Elem.M;
								bnr = (mp->PBrms[HOMmax+Quad])/(mp->PBpar[HOMmax+Quad]);
								anr = (mp->PBrms[HOMmax-Quad])/(mp->PBpar[HOMmax+Quad]);
								break;
							}
						set_bnr_rms_type(Quad, Quad, bnr, anr, new_rnd); //introduzido por Fernando em 30/11/2011
						misalign_rms_type(Quad, dx, dy, dr_deg, new_rnd);
					} else
						misalign_sys_type(Quad, dx, dy, dr_deg);
				} else if (strcmp("sext", Name) == 0) {
					printf("misaligning sextupoles:  dx = %e, dy = %e, dr = %e\n",
							dx, dy, dr);
					if (rms){
						// Inserido por Fernando em 30/11/2011
						for(ki = 1; ki <= globval.Cell_nLoc; ki++)
							if ((Cell[ki].Elem.Pkind == Mpole) && (Cell[ki].Elem.M->n_design == Sext)) {
								mp = Cell[ki].Elem.M;
								bnr = (mp->PBrms[HOMmax+Sext])/(mp->PBpar[HOMmax+Sext]);
								anr = (mp->PBrms[HOMmax-Sext])/(mp->PBpar[HOMmax+Sext]);
								break;
							}
						set_bnr_rms_type(Sext, Sext, bnr, anr, new_rnd);  //introduzido por Fernando em 30/11/2011
						misalign_rms_type(Sext, dx, dy, dr_deg, new_rnd);
					} else
						misalign_sys_type(Sext, dx, dy, dr_deg);
				} else if (strcmp("bpm", Name) == 0) {
					printf("misaligning bpms:        dx = %e, dy = %e, dr = %e\n",
							dx, dy, dr);
					if (rms)
						misalign_rms_fam(globval.bpm, dx, dy, dr_deg, new_rnd);
					else
						misalign_sys_fam(globval.bpm, dx, dy, dr_deg);
				} else {
					Fnum = ElemIndex(Name);
					if(Fnum > 0) {
						printf("misaligning all %s:  dx = %e, dy = %e, dr = %e\n",
								Name, dx, dy, dr);
						if (rms)
							misalign_rms_fam(Fnum, dx, dy, dr_deg, new_rnd);
						else
							misalign_sys_fam(Fnum, dx, dy, dr_deg);
					} else
						printf("LoadAlignTol: undefined element %s\n", Name);
				}
			}
		} else
			printf("%s", line);
	}

	fclose(fp);
}

/****************************************************************************/
/* void set_aper_elem(const int Fnum, const int Knum, 
		   const double Dxmin, const double Dxmax, 
		   const double Dymin, const double Dymax) 

   Purpose:
      Define vacuum chamber to the element  
      

   Input:
      none

   Output:
       none

   Return:
       none

   Global variables:
       none

   Specific functions:
       none

   Comments:
      Add k=0 for the default start point "start" in tracy.

****************************************************************************/
// apertures

void set_aper_elem(const int Fnum, const int Knum, 
		   const double Dxmin, const double Dxmax, 
		   const double Dymin, const double Dymax) 
{ 
  int  k; 
  
  if(Fnum==0 && Knum==0)
    k=0;
  else
    k = Elem_GetPos(Fnum, Knum);
     
    Cell[k].maxampl[X_][0] = Dxmin; 
    Cell[k].maxampl[X_][1] = Dxmax; 
    Cell[k].maxampl[Y_][0] = Dymin; 
    Cell[k].maxampl[Y_][1] = Dymax; 
 } 

 /****************************************************************************/
/* void set_aper_fam(const int Fnum,
		  const double Dxmin, const double Dxmax, 
		  const double Dymin, const double Dymax)
   Purpose:
      Define vacuum chamber to the family "Fnum"  
      

   Input:
      none
      
   Output:
       none

   Return:
       none

   Global variables:
       none

   Specific functions:
       none

   Comments:
       Enumeration of special variables:
                 enum { All = 0, Dip = 1, Quad = 2, Sext = 3, Oct = 4, Dec = 5, Dodec = 6 }

****************************************************************************/
void set_aper_fam(const int Fnum,
		  const double Dxmin, const double Dxmax, 
		  const double Dymin, const double Dymax)
{
  int k;

  for (k = 1; k <= GetnKid(Fnum); k++)
    set_aper_elem(Fnum, k, Dxmin, Dxmax, Dymin, Dymax);
}

/****************************************************************************/
/* void set_aper_type(const int type, const double Dxmin, const double Dxmax, 
		   const double Dymin, const double Dymax) 

   Purpose:
      Define vacuum chamber to the elements with the same "type", either for "all" element, or "quad","sext", etc.  
      

   Input:
       none
       
   Output:
       none

   Return:
       none

   Global variables:
       none

   Specific functions:
       none

   Comments:
      1) Enumeration of special variables:
                 enum { All = 0, Dip = 1, Quad = 2, Sext = 3, Oct = 4, Dec = 5, Dodec = 6 }
      2) change the start point from k=1 to k=0, in order to set the aperture for
         the default start point "begin" which is not a lattice element.

****************************************************************************/
void set_aper_type(const int type, const double Dxmin, const double Dxmax, 
		   const double Dymin, const double Dymax)
{
  long int   k;

  if (type >= All && type <= HOMmax) 
    {
 //   for(k = 1; k <= globval.Cell_nLoc; k++)
      for(k = 0; k <= globval.Cell_nLoc; k++)
        if (((Cell[k].Elem.Pkind == Mpole) &&
	   (Cell[k].Elem.M->n_design == type)) || (type == All))
	set_aper_elem(Cell[k].Fnum, Cell[k].Knum, Dxmin, Dxmax, Dymin, Dymax);
     } 
  else
    printf("set_aper_type: bad design type %d\n", type);
}


/****************************************************************************/
/* void LoadApers(const char *AperFile, const double scl_x, const double scl_y) 

   Purpose:
      Read vacuum chamber from file "AperFile", and scale the values with scl_x and scl_y.
      Assign vacuum chamber to all elements, if there are definition of 
      chamber at quad and sext, then assign the specific values at these elements.
      

   Input:
       AperFile:  file with chamber definition
       scl_x   :  scale factor of x limitation
       scl_y   : scale factor of y limitation

   Output:
       none

   Return:
       none

   Global variables:
       none

   Specific functions:
       none

   Comments:
       Enumeration of special variables:
                 enum { All = 0, Dip = 1, Quad = 2, Sext = 3, Oct = 4, Dec = 5, Dodec = 6 }
		 
    10/2010   Comments by Jianfeng Zhang

****************************************************************************/
void LoadApers(const char *AperFile, const double scl_x, const double scl_y) 
{
  char    line[max_str], Name[max_str];
  int     Fnum; 
  double  dxmin, dxmax, dymin, dymax;  // min and max x and apertures
  FILE    *fp;

  bool  prt = true;

  fp = file_read(AperFile);

  printf("\n");
  printf("...Load and Set Apertures.\n");

  while (fgets(line, max_str, fp) != NULL) {
    if (strstr(line, "#") == NULL) 
    {
      sscanf(line,"%s %lf %lf %lf %lf",
	     Name, &dxmin, &dxmax, &dymin, &dymax);
      dxmin *= scl_x; 
      dxmax *= scl_x; 
      dymin *= scl_y; 
      dymax *= scl_y;
      
      if (strcmp("all", Name)==0) {
	if(prt)
	  printf("setting all apertures to"
		 " dxmin = %e, dxmax = %e, dymin = %e, dymax = %e\n",
		 dxmin, dxmax, dymin, dymax);
	set_aper_type(All, dxmin, dxmax, dymin, dymax);
	//	ini_aper(dxmin, dxmax, dymin, dymax); 
       }
        
      else if (strcmp("quad", Name)==0) {
	if(prt)
	  printf("setting apertures at all quads to"
		 " dxmin = %e, dxmax = %e, dymin = %e, dymax = %e\n",
		 dxmin, dxmax, dymin, dymax);  
	set_aper_type(Quad, dxmin, dxmax, dymin, dymax);
       }
        
      else if (strcmp("sext", Name) == 0) {
	if(prt)
	  printf("setting apertures at all sextupoles to"
		 " dxmin = %e, dxmax = %e, dymin = %e, dymax = %e\n",
		 dxmin, dxmax, dymin, dymax);
	set_aper_type(Sext, dxmin, dxmax, dymin, dymax);
       }
        
      else {
	Fnum = ElemIndex(Name);
	if(Fnum > 0) {
	  if(prt)
	    printf("setting apertures at all %s to"
		   " dxmin = %e, dxmax = %e, dymin = %e, dymax = %e\n",
		   Name, dxmin, dxmax, dymin, dymax);
	  set_aper_fam(Fnum, dxmin, dxmax, dymin, dymax);
	  } 
	 else 
	  printf("LoadApers: lattice does not contain element %s\n", Name);
      }
    } 
   else
      printf("%s", line);
  }
    
  fclose(fp);
}


double get_L(const int Fnum, const int Knum)
{
  return Cell[Elem_GetPos(Fnum, Knum)].Elem.PL;
}


void set_L(const int Fnum, const int Knum, const double L)
{

  Cell[Elem_GetPos(Fnum, Knum)].Elem.PL = L;
}


void set_L(const int Fnum, const double L)
{
  int  k;

  for (k = 1; k <= GetnKid(Fnum); k++)
    set_L(Fnum, k, L);
}


void set_dL(const int Fnum, const int Knum, const double dL)
{

  Cell[Elem_GetPos(Fnum, Knum)].Elem.PL += dL;
}


// multipole components
/*************************************************************
void get_bn_design_elem(const int Fnum, const int Knum,
			const int n, double &bn, double &an)
			
   Purpose:
       Get the n-th order design value (bn, an) of the element
       with family index "Fnum" and the kid number "Knum".
       
  Input:
      Fnum:   family index
      Knum:   kid index
         n:   n-th design order of the element
	bn:   bn component of the element
	an:   an component of the element 

 Output:
    None
    
 Return:
          bn, an
	 
 Comments:
 
            11/2010 comments by jianfeng Zhang 
**************************************************************/
void get_bn_design_elem(const int Fnum, const int Knum,
			const int n, double &bn, double &an)
{
  elemtype  elem;

  elem = Cell[Elem_GetPos(Fnum, Knum)].Elem;

  bn = elem.M->PBpar[HOMmax+n]; an = elem.M->PBpar[HOMmax-n];
}


void get_bnL_design_elem(const int Fnum, const int Knum,
			 const int n, double &bnL, double &anL)
{
  elemtype  elem;

  elem = Cell[Elem_GetPos(Fnum, Knum)].Elem;

  bnL = elem.M->PBpar[HOMmax+n]; anL = elem.M->PBpar[HOMmax-n];

  if (elem.PL != 0.0) {
    bnL *= elem.PL; anL *= elem.PL;
  }
}


void set_bn_design_elem(const int Fnum, const int Knum,
			const int n, const double bn, const double an)
{
  elemtype  elem;

  elem = Cell[Elem_GetPos(Fnum, Knum)].Elem;

  elem.M->PBpar[HOMmax+n] = bn; elem.M->PBpar[HOMmax-n] = an;

  Mpole_SetPB(Fnum, Knum, n); Mpole_SetPB(Fnum, Knum, -n);
}


void set_dbn_design_elem(const int Fnum, const int Knum,
			 const int n, const double dbn, const double dan)
{
  elemtype  elem;

  elem = Cell[Elem_GetPos(Fnum, Knum)].Elem;

  elem.M->PBpar[HOMmax+n] += dbn; elem.M->PBpar[HOMmax-n] += dan;

  Mpole_SetPB(Fnum, Knum, n); Mpole_SetPB(Fnum, Knum, -n);
}


void set_bn_design_fam(const int Fnum,
		       const int n, const double bn, const double an)
{
  int k;

  for (k = 1; k <= GetnKid(Fnum); k++)
    set_bn_design_elem(Fnum, k, n, bn, an);
}


void set_dbn_design_fam(const int Fnum,
			const int n, const double dbn, const double dan)
{
  int k;

  for (k = 1; k <= GetnKid(Fnum); k++)
    set_dbn_design_elem(Fnum, k, n, dbn, dan);
}


void set_bnL_design_elem(const int Fnum, const int Knum,
			 const int n, const double bnL, const double anL)
{
  elemtype  elem;

  elem = Cell[Elem_GetPos(Fnum, Knum)].Elem;

  if (elem.PL != 0.0) {
    elem.M->PBpar[HOMmax+n] = bnL/elem.PL;
    elem.M->PBpar[HOMmax-n] = anL/elem.PL;
  } else {
    // thin kick
    elem.M->PBpar[HOMmax+n] = bnL; elem.M->PBpar[HOMmax-n] = anL;
  }

  Mpole_SetPB(Fnum, Knum, n); Mpole_SetPB(Fnum, Knum, -n);
}


void set_dbnL_design_elem(const int Fnum, const int Knum,
			  const int n, const double dbnL, const double danL)
{
  elemtype  elem;

  elem = Cell[Elem_GetPos(Fnum, Knum)].Elem;

  if (elem.PL != 0.0) {
    elem.M->PBpar[HOMmax+n] += dbnL/elem.PL;
    elem.M->PBpar[HOMmax-n] += danL/elem.PL;
  } else {
    // thin kick
    elem.M->PBpar[HOMmax+n] += dbnL; elem.M->PBpar[HOMmax-n] += danL;
  }

  Mpole_SetPB(Fnum, Knum, n); Mpole_SetPB(Fnum, Knum, -n);
}


void set_dbnL_design_fam(const int Fnum,
			 const int n, const double dbnL, const double danL)
{
  int k;

  for (k = 1; k <= GetnKid(Fnum); k++)
    set_bnL_design_elem(Fnum, k, n, dbnL, danL);
}


void set_bnL_design_fam(const int Fnum,
			const int n, const double bnL, const double anL)
{
  int k;

  for (k = 1; k <= GetnKid(Fnum); k++)
    set_bnL_design_elem(Fnum, k, n, bnL, anL);
}


void set_bnL_design_type(const int type,
			 const int n, const double bnL, const double anL)
{
  long int  k;

  if ((type >= Dip) && (type <= HOMmax)) {
    for (k = 1; k <= globval.Cell_nLoc; k++)
      if ((Cell[k].Elem.Pkind == Mpole) && (Cell[k].Elem.M->n_design == type))
	set_bnL_design_elem(Cell[k].Fnum, Cell[k].Knum, n, bnL, anL);
  } else 
    printf("Bad type argument to set_bnL_design_type()\n");
}

/***********************************************************************
void set_bnL_sys_elem(const int Fnum, const int Knum,
		      const int n, const double bnL, const double anL)
			 
   Purpose:
       Set field error to the kid of a family, errors are absolute
        integrated field strength, replace the previous value!!!
	
   
   Input:
      Fnum           family index
      Knum           kids index   
      n              order of the error
      bnL            absolute integrated B component for the n-th error
      anL            absolute integrated A component for the n-th error
      	    

     
   Output:
      None
      
  Return:
      None
      
  Global variables
      None
   
  Specific functions:
     None    
     
 Comments:
     None
**********************************************************************/
void set_bnL_sys_elem(const int Fnum, const int Knum,
		      const int n, const double bnL, const double anL)
{
  elemtype  elem;

  const bool  prt = false;

  elem = Cell[Elem_GetPos(Fnum, Knum)].Elem;

  if (elem.PL != 0.0) {
    elem.M->PBsys[HOMmax+n] = bnL/elem.PL;
    elem.M->PBsys[HOMmax-n] = anL/elem.PL;
  } else {
    // thin kick
    elem.M->PBsys[HOMmax+n] = bnL; elem.M->PBsys[HOMmax-n] = anL;
  }
  
  Mpole_SetPB(Fnum, Knum, n);    //set for Bn component
  Mpole_SetPB(Fnum, Knum, -n);   //set for An component

  if (prt)
    printf("set the n=%d component of %s to %e %e\n",
	   n, Cell[Elem_GetPos(Fnum, Knum)].Elem.PName,
	   bnL, elem.M->PBsys[HOMmax+n]);
}

/***********************************************************************
void set_bnL_sys_fam(const int Fnum,
		     const int n, const double bnL, const double anL)
			 
   Purpose:
       Set field error to all the kids of a family,errors are absolute
        integrated field strength
	
	
   
   Input:
      Fnum           family index
      n              order of the error
      bnL            absolute integrated B component for the n-th error
      anL            absolute integrated A component for the n-th error
      	    

     
   Output:
      None
      
  Return:
      None
      
  Global variables
      None
   
  Specific functions:
     None    
     
 Comments:
     None
**********************************************************************/
void set_bnL_sys_fam(const int Fnum,
		     const int n, const double bnL, const double anL)
{
  int k;

  for (k = 1; k <= GetnKid(Fnum); k++)
    set_bnL_sys_elem(Fnum, k, n, bnL, anL);
}

/***********************************************************************
void set_bnL_sys_type(const int type,
		      const int n, const double bnL, const double anL)
			 
   Purpose:
       Set field error to the type, errors are absolute
        integrated field strength 
   
	
	
   Input:
      type          type name 
      n             order of the error
      bnL           absolute integrated B component for the n-th error
      anL           absolute integrated A component for the n-th error
      	    

     
   Output:
      None
      
  Return:
      None
      
  Global variables
      None
   
  Specific functions:
     None    
     
 Comments:
      Attension:
          This code has a bug, in the for loop, when looking for the 
	  quadrupole family, it will also find the skew quadrupole family,
	  since their Pkind == Mpole & n_design =2.
	  It means the multipole error is additionally set N times,
	  and N is the number of skew quadrupole in the lattice.
	  
	         10/2010   Comments by Jianfeng Zhang
     
**********************************************************************/
void set_bnL_sys_type(const int type,
		      const int n, const double bnL, const double anL)
{
  long int   k;

  if (type >= Dip && type <= HOMmax) {
    for(k = 1; k <= globval.Cell_nLoc; k++)
      if ((Cell[k].Elem.Pkind == Mpole) && (Cell[k].Elem.M->n_design == type))
	set_bnL_sys_elem(Cell[k].Fnum, Cell[k].Knum, n, bnL, anL);
  } else
    printf("Bad type argument to set_bnL_sys_type()\n");
}


void set_bnL_rms_elem(const int Fnum, const int Knum,
		      const int n, const double bnL, const double anL,
		      const bool new_rnd)
{
  elemtype  elem;

  bool  normal = true, prt = false;
  
  elem = Cell[Elem_GetPos(Fnum, Knum)].Elem;

  if (elem.PL != 0.0) {
    elem.M->PBrms[HOMmax+n] = bnL/elem.PL;
    elem.M->PBrms[HOMmax-n] = anL/elem.PL;
  } else {
    // thin kick
    elem.M->PBrms[HOMmax+n] = bnL; elem.M->PBrms[HOMmax-n] = anL;
  }

  if(new_rnd){
    if (normal) {
      elem.M->PBrnd[HOMmax+n] = normranf();
      elem.M->PBrnd[HOMmax-n] = normranf();
    } else {
      elem.M->PBrnd[HOMmax+n] = ranf(); elem.M->PBrnd[HOMmax-n] = ranf();
    }
  }
  
  if (prt)
    printf("set_bnL_rms_elem:  Fnum = %d, Knum = %d"
	   ", bnL = %e, anL = %e %e %e\n",
	   Fnum, Knum, bnL, anL,
	   elem.M->PBrms[HOMmax+n], elem.M->PBrms[HOMmax-n]);

  Mpole_SetPB(Fnum, Knum, n); Mpole_SetPB(Fnum, Knum, -n);
}


void set_bnL_rms_fam(const int Fnum,
		     const int n, const double bnL, const double anL,
		     const bool new_rnd)
{
  int k;

  for (k = 1; k <= GetnKid(Fnum); k++)
    set_bnL_rms_elem(Fnum, k, n, bnL, anL, new_rnd);
}


void set_bnL_rms_type(const int type,
		      const int n, const double bnL, const double anL,
		      const bool new_rnd)
{
  long int   k;
  
  if (type >= Dip && type <= HOMmax) {
    for(k = 1; k <= globval.Cell_nLoc; k++)
      if ((Cell[k].Elem.Pkind == Mpole) && (Cell[k].Elem.M->n_design == type))
	set_bnL_rms_elem(Cell[k].Fnum, Cell[k].Knum, n, bnL, anL, new_rnd);
  } else
    printf("Bad type argument to set_bnL_rms_type()\n");
}

/***********************************************************************
void set_bnr_sys_elem(const int Fnum, const int Knum,
		      const int n, const double bnr, const double anr)
			 
   Purpose:
       Set field error to the kid of a family, errors are relative to
        design values for (Dip, Quad, Sext, ...)
   
	If r0 != 0, call this function
	
   Input:
      Fnum           family index
      Knum           kids index   
      n              order of the error
      bnL            relative integrated B component for the n-th error
      anL            relative integrated A component for the n-th error
      	    

     
   Output:
      None
      
  Return:
      None
      
  Global variables
      None
   
  Specific functions:
     None    
     
 Comments:
      !!!!!!!!!
      This function desn't work for dipole, since   
      mp->PBpar[HOMmax+1] is not assigned a value when dipole is 
      read in t2lat.cc.  
      
      comments by Jianfeng Zhang  10/2010      
**********************************************************************/
void set_bnr_sys_elem(const int Fnum, const int Knum,
		      const int n, const double bnr, const double anr)
{
  int        nd;
  MpoleType  *mp;
  bool prt = false;

  mp = Cell[Elem_GetPos(Fnum, Knum)].Elem.M; nd = mp->n_design;
  // errors are relative to design values for (Dip, Quad, Sext, ...)
  mp->PBsys[HOMmax+n] = bnr*mp->PBpar[HOMmax+nd];
  mp->PBsys[HOMmax-n] = anr*mp->PBpar[HOMmax+nd];
  //update the total field strength PB and maximum order p_order
  Mpole_SetPB(Fnum, Knum, n); Mpole_SetPB(Fnum, Knum, -n);

  if (prt)
    printf("set the n=%d component of %s to %e %e %e\n",
	   n, Cell[Elem_GetPos(Fnum, Knum)].Elem.PName,
	   bnr, mp->PBpar[HOMmax+nd], mp->PBsys[HOMmax+n]);
}

/***********************************************************************
void set_bnr_sys_fam(const int Fnum,
		     const int n, const double bnr, const double anr)
			 
   Purpose:
       Set field error to all the kids of a family, errors are relative
        to design values for (Dip, Quad, Sext, ...).
	
	If r0 != 0, call this function
   
   Input:
      Fnum           family index
      n              order of the error
      bnL            relative integrated B component for the n-th error
      anL            relative integrated A component for the n-th error
      	    

     
   Output:
      None
      
  Return:
      None
      
  Global variables
      None
   
  Specific functions:
     None    
     
 Comments:
     None
**********************************************************************/
void set_bnr_sys_fam(const int Fnum,
		     const int n, const double bnr, const double anr)
{
  int k;

  for (k = 1; k <= GetnKid(Fnum); k++)
    set_bnr_sys_elem(Fnum, k, n, bnr, anr);
}

/***********************************************************************
void set_bnr_sys_type(const int type,
		      const int n, const double bnr, const double anr)
			 
   Purpose:
       Set field error to the type, errors are relative
        to design values for (Dip, Quad, Sext, ...)
   
	If r0 != 0, call this function
	
   Input:
      type          type name 
      n             order of the error
      bnL           relative integrated B component for the n-th error
      anL           relative integrated A component for the n-th error
      	    

     
   Output:
      None
      
  Return:
      None
      
  Global variables
      None
   
  Specific functions:
     None    
     
 Comments:
     None
**********************************************************************/
void set_bnr_sys_type(const int type,
		      const int n, const double bnr, const double anr)
{
  long int   k;

  if (type >= Dip && type <= HOMmax) {
    for(k = 1; k <= globval.Cell_nLoc; k++)
      if ((Cell[k].Elem.Pkind == Mpole) && (Cell[k].Elem.M->n_design == type))
	set_bnr_sys_elem(Cell[k].Fnum, Cell[k].Knum, n, bnr, anr);
  } else
    printf("Bad type argument to set_bnr_sys_type()\n");
}

/***********************************************************************
void set_bnr_rms_elem(const int Fnum, const int Knum,
		      const int n, const double bnr, const double anr,
		      const bool new_rnd)
			 
   Purpose:
       Update rms field error PBrms and random value of the rms field error PBrnd 
       to a elment, errors are relative to design values PBpar for (Dip, Quad, 
       Sext, ...).
	
	If r0 != 0, call this function
   
   Input:
      Fnum           family index
      Knum           kid index
      n              order of the error
      bnL            relative integrated B component for the n-th error
      anL            relative integrated A component for the n-th error
      new_rnd        true, generate random value for the rms field error
                     false, NOT generate random value for the rms field error	    
		      
   Output:
      None
      
  Return:
      None
      
  Global variables
      None
   
  Specific functions:
     None    
     
 Comments:
     None
**********************************************************************/
void set_bnr_rms_elem(const int Fnum, const int Knum,
		      const int n, const double bnr, const double anr,
		      const bool new_rnd)
{
  int        nd;
  double     KN;
  MpoleType  *mp;

  bool  normal = true, prt = false;
  
  mp = Cell[Elem_GetPos(Fnum, Knum)].Elem.M; nd = mp->n_design;

  // Alterado em 24/11/2011 - erros do dipolo não eram atribuidos
  // errors are relative to design values for (Dip, Quad, Sext, ...)
  if(nd == Dip)
        KN = mp->Pirho; /*dipole strength*/
  	  else
  	    KN = mp->PBpar[HOMmax+nd];/*other multipoles*/


  mp->PBrms[HOMmax+n] = bnr*KN;
  mp->PBrms[HOMmax-n] = anr*KN;
  //Termino da alteração

  if(new_rnd){
    if (normal) {
      mp->PBrnd[HOMmax+n] = normranf(); mp->PBrnd[HOMmax-n] = normranf();
    } else {
      mp->PBrnd[HOMmax+n] = ranf(); mp->PBrnd[HOMmax-n] = ranf();
    }
  }
  
  if (prt)
    printf("set_bnr_rms_elem:  Fnum = %d, Knum = %d"
	   ", bnr = %e, anr = %e %e %e\n",
	   Fnum, Knum, bnr, anr, mp->PBrms[HOMmax+n], mp->PBrms[HOMmax-n]);

  Mpole_SetPB(Fnum, Knum, n); Mpole_SetPB(Fnum, Knum, -n);
}

/***********************************************************************
void set_bnr_rms_fam(const int Fnum,
		     const int n, const double bnr, const double anr,
		     const bool new_rnd)
			 
   Purpose:
       Update rms field error PBrms and random value of the rms field error PBrnd 
       to all the kid elments in a family, errors are relative to design values PBpar 
       for (Dip, Quad, Sext, ...).
	
	If r0 != 0, call this function
   
   Input:
      Fnum           family index
      n              order of the error
      bnr            relative integrated B component for the n-th error
      anr            relative integrated A component for the n-th error
      new_rnd        true, generate random value for the rms field error
                     false, NOT generate random value for the rms field error	    
		      
   Output:
      None
      
  Return:
      None
      
  Global variables
      None
   
  Specific functions:
     None    
     
 Comments:
     None
**********************************************************************/
void set_bnr_rms_fam(const int Fnum,
		     const int n, const double bnr, const double anr,
		     const bool new_rnd)
{
  int k;

  for (k = 1; k <= GetnKid(Fnum); k++)
    set_bnr_rms_elem(Fnum, k, n, bnr, anr, new_rnd);
}

/***********************************************************************
void set_bnr_rms_type(const int type,
		      const int n, const double bnr, const double anr,
		      const bool new_rnd)
			 
   Purpose:
       Update rms field error PBrms and random value of the rms field error PBrnd 
       to all the kid elments in a family, errors are relative to design values PBpar 
       for (Dip, Quad, Sext, ...).
	
	If r0 != 0, call this function
   
   Input:
      type           type of the elment,maybe 'dipole', 'quadrupole', 'sextupole', 'all'
      n              order of the error
      bnr            relative integrated B component for the n-th error
      anr            relative integrated A component for the n-th error
      new_rnd        true, generate random value for the rms field error
                     false, NOT generate random value for the rms field error	    
		      
   Output:
      None
      
  Return:
      None
      
  Global variables
      None
   
  Specific functions:
     None    
     
 Comments:
     None
**********************************************************************/
void set_bnr_rms_type(const int type,
		      const int n, const double bnr, const double anr,
		      const bool new_rnd)
{
  long int   k;
  
  if (type >= Dip && type <= HOMmax) {
    for(k = 1; k <= globval.Cell_nLoc; k++)
      if ((Cell[k].Elem.Pkind == Mpole) && (Cell[k].Elem.M->n_design == type))
	set_bnr_rms_elem(Cell[k].Fnum, Cell[k].Knum, n, bnr, anr, new_rnd);
  } else
    printf("Bad type argument to set_bnr_rms_type()\n");
}


double get_Wiggler_BoBrho(const int Fnum, const int Knum)
{
  return Cell[Elem_GetPos(Fnum, Knum)].Elem.W->BoBrhoV[0];
}

/****************************************************************************/
/* void set_Wiggler_BoBrho(const int Fnum, const int Knum, const double BoBrhoV)

   Purpose:
       Set the integrated field strength for the specific element in the family 
       of wiggers.       
       
   Input:
       Fnum     family name
       BoBrhoV  integrated field strength 

   Output:
      none

   Return:
       none

   Global variables:
       none

   Specific functions:
       none

   Comments:
       Called by set_Wiggler_BoBrho(const int Fnum, const double BoBrhoV)
       
****************************************************************************/
void set_Wiggler_BoBrho(const int Fnum, const int Knum, const double BoBrhoV)
{
  Cell[Elem_GetPos(Fnum, Knum)].Elem.W->BoBrhoV[0] = BoBrhoV;
  Cell[Elem_GetPos(Fnum, Knum)].Elem.W->PBW[HOMmax+Quad] = -sqr(BoBrhoV)/2.0;
  Wiggler_SetPB(Fnum, Knum, Quad);
}

/****************************************************************************/
/* void set_Wiggler_BoBrho(const int Fnum, const double BoBrhoV)

   Purpose:
       Set the integrated field strength for the family of wiggers       

   Input:
       Fnum     family name
       BoBrhoV  integrated field strength 

   Output:
      none

   Return:
       none

   Global variables:
       none

   Specific functions:
       none

   Comments:
       none
       
****************************************************************************/
void set_Wiggler_BoBrho(const int Fnum, const double BoBrhoV)
{
  int  k;

  for (k = 1; k <= GetnKid(Fnum); k++)
    set_Wiggler_BoBrho(Fnum, k, BoBrhoV);
}

/****************************************************************************/
/* void set_ID_scl(const int Fnum, const int Knum, const double scl)

   Purpose:
       Set the scale factor for the spicified element of wigger, insertion devices, 
       or fieldMap.
       Called by set_ID_scl(const int Fnum, const double scl).        

   Input:
       Fnum  family name
       Knum  kid index of Fname
       scl   scaling factor

   Output:
      none

   Return:
       none

   Global variables:
       none

   Specific functions:
       none

   Comments:
       none
       
****************************************************************************/
void set_ID_scl(const int Fnum, const int Knum, const double scl)
{
  int           k;
  WigglerType*  W;

  switch (Cell[Elem_GetPos(Fnum, Knum)].Elem.Pkind) {
  case Wigl:
    // scale the ID field
    W = Cell[Elem_GetPos(Fnum, Knum)].Elem.W;
    for (k = 0; k < W->n_harm; k++) {
      W->BoBrhoH[k] = scl*ElemFam[Fnum-1].ElemF.W->BoBrhoH[k];
      W->BoBrhoV[k] = scl*ElemFam[Fnum-1].ElemF.W->BoBrhoV[k];
    }
    break;
  case Insertion:
    Cell[Elem_GetPos(Fnum, Knum)].Elem.ID->scaling1 = scl;
    break;
  case FieldMap:
    Cell[Elem_GetPos(Fnum, Knum)].Elem.FM->scl = scl;
    break;
  default:
    cout << "set_ID_scl: unknown element type" << endl;
    exit_(1);
    break;
  }
}

/****************************************************************************/
/* void set_ID_scl(const int Fnum, const double scl)

   Purpose:
       Set the scale factor for the specified wigger, insertion devices, or fieldMap.        

   Input:
       Fnum  family name
       scl   scaling factor

   Output:
      none

   Return:
       none

   Global variables:
       none

   Specific functions:
       none

   Comments:
       none
       
****************************************************************************/
void set_ID_scl(const int Fnum, const double scl)
{
  int  k;

  for (k = 1; k <= GetnKid(Fnum); k++)
    set_ID_scl(Fnum, k, scl);
}

/***********************************************************************
void SetFieldValues_fam(const int Fnum, const bool rms, const double r0,
			const int n, const double Bn, const double An,
			const bool new_rnd)
			 
   Purpose:
       Set field error to all the kids in a family 
   
   Input:
      Fnum            family name 
      rms             flag to set rms field error
      r0              radius at which error is measured
      n               order of the error
      Bn              integrated B component for the n-th error
      An              integrated A component for the n-th error
      new_rnd         bool flag to set new random number	    

     
   Output:
      None
      
  Return:
      None
      
  Global variables
      None
   
  Specific functions:
     None    
     
 Comments:
     None
**********************************************************************/
void SetFieldValues_fam(const int Fnum, const bool rms, const double r0,
			const int n, const double Bn, const double An,
			const bool new_rnd)
{
  int     N;
  double  bnr, anr;

  N = Cell[Elem_GetPos(Fnum, 1)].Elem.M->n_design;
  if (r0 == 0.0) {
    // input is: (b_n L), (a_n L)
    if(rms)
      set_bnL_rms_fam(Fnum, n, Bn, An, new_rnd);
    else
      set_bnL_sys_fam(Fnum, n, Bn, An);
  } else {
    bnr = Bn/pow(r0, n-N); anr = An/pow(r0, n-N);
    if(rms)
      set_bnr_rms_fam(Fnum, n, bnr, anr, new_rnd);
    else
      set_bnr_sys_fam(Fnum, n, bnr, anr);
  }
}


/***********************************************************************
void SetFieldValues_type(const int N, const bool rms, const double r0,
			 const int n, const double Bn, const double An,
			 const bool new_rnd)
			 
   Purpose:
       Set field error to the type 
   
   Input:
      N            type name 
      rms          flag to set rms field error
      r0           radius at which error is measured
      n            order of the error
      Bn           integrated B component for the n-th error
      An           integrated A component for the n-th error
      new_rnd      bool flag to set new random number	    

     
   Output:
      None
      
  Return:
      None
      
  Global variables
      None
   
  Specific functions:
     None    
     
 Comments:
     None
**********************************************************************/
void SetFieldValues_type(const int N, const bool rms, const double r0,
			 const int n, const double Bn, const double An,
			 const bool new_rnd)
{
  double  bnr, anr;

  if (r0 == 0.0) {
    // input is: (b_n L), (a_n L)
    if(rms)
      set_bnL_rms_type(N, n, Bn, An, new_rnd);
    else
      set_bnL_sys_type(N, n, Bn, An);
  } else {
    bnr = Bn/pow(r0, n-N); anr = An/pow(r0, n-N);
    if(rms)
      set_bnr_rms_type(N, n, bnr, anr, new_rnd);
    else
      set_bnr_sys_type(N, n, bnr, anr);
  }
}


/***********************************************************************
void SetFieldErrors(const char *name, const bool rms, const double r0,
		    const int n, const double Bn, const double An,
		    const bool new_rnd) 
		    
   Purpose:
       Set field error to the type or element
   
   Input:
      name         type name or element name
      rms          flag to set rms field error
      r0           radius at which error is measured
      n            order of the error
      Bn           integrated B component for the n-th error
      An           integrated A component for the n-th error
      new_rnd      flag to set new random number for the error	    

     
   Output:
      None
      
  Return:
      None
      
  Global variables
      None
   
  Specific functions:
     None    
     
 Comments:
       10/2010   Comments by Jianfeng Zhang
**********************************************************************/
void SetFieldErrors(const char *name, const bool rms, const double r0,
		    const int n, const double Bn, const double An,
		    const bool new_rnd) 
{
  int     Fnum;

  if (strcmp("all", name) == 0) {
    printf("all: not yet implemented\n");
  } else if (strcmp("dip", name) == 0) {
    SetFieldValues_type(Dip, rms, r0, n, Bn, An, new_rnd);
  } else if (strcmp("quad", name) == 0) {
    SetFieldValues_type(Quad, rms, r0, n, Bn, An, new_rnd);
  } else if (strcmp("sext", name) == 0) {
    SetFieldValues_type(Sext, rms, r0, n, Bn, An, new_rnd);
  } else {
    Fnum = ElemIndex(name);
    if(Fnum > 0)
      SetFieldValues_fam(Fnum, rms, r0, n, Bn, An, new_rnd);
    else 
      printf("SetFieldErrors: undefined element %s\n", name);
  }
}



/********************************************************************
char* get_prm(void)

    Purpose:
        Find the stoke talbe symbol "\t" in the string, and 
	then saved and return the position of the pointer.
     
    symbol for end of the line:
	\n   
    Comments:
       10/2010  By Jianfeng Zhang
        Fix the bug for reading the end of line symbol "\n" , "\r",'\r\n' 
	at different operation system
*********************************************************************/
char* get_prm(void)
{
  char  *prm;

  prm = strtok(NULL, " \t");

  //if ((prm == NULL) || (strcmp(prm, "\r\n") == 0)) {
  if ((prm == NULL) || (strcmp(prm, "\n") == 0)|| (strcmp(prm, "\r") == 0)|| (strcmp(prm, "\r\n") == 0)) {
    printf("get_prm: incorrect format\n");
    exit_(1);
  }

  return prm;
}


void LoadFieldErr(const char *FieldErrorFile, const bool Scale_it,
		  const double Scale, const bool new_rnd) 
{  
  bool    rms, set_rnd;
  char    line[max_str], name[max_str], type[max_str], *prm;
  int     k, n, seed_val;
  double  Bn, An, r0;
  FILE    *inf;

  const bool  prt = true;

  inf = file_read(FieldErrorFile);

  set_rnd = false; 
  printf("\n");
  while (fgets(line, max_str, inf) != NULL) {
    if (strstr(line, "#") == NULL) {
      // check for whether to set new seed
      sscanf(line, "%s", name); 
      if (strcmp("seed", name) == 0) {
	set_rnd = true;
	sscanf(line, "%*s %d", &seed_val); 
//	printf("setting random seed to %d\n", seed_val);
//	iniranf(seed_val); 
	printf("\n");
	printf("setting new random numbers (%d)\n", seed_val);
      } else {
	sscanf(line, "%*s %s %lf", type, &r0);
	printf("%-4s %3s %7.1le", name, type, r0);
	rms = (strcmp("rms", type) == 0)? true : false;
	if (rms && !set_rnd) {
	  printf("LoadFieldErr: seed not defined\n");
	  exit_(1);
	}
	// skip first three parameters
	strtok(line, " \t");
	for (k = 1; k <= 2; k++)
	  strtok(NULL, " \t");
	while (((prm = strtok(NULL, " \t")) != NULL) &&
	       (strcmp(prm, "\r\n") != 0)) {
	  sscanf(prm, "%d", &n);
	  prm = get_prm();
	  sscanf(prm, "%lf", &Bn);
	  prm = get_prm(); 
	  sscanf(prm, "%lf", &An);
	  if (Scale_it)
	    {Bn *= Scale; An *= Scale;}
	  if (prt)
	    printf(" %2d %9.1e %9.1e\n", n, Bn, An);
	  // convert to normalized multipole components
	  SetFieldErrors(name, rms, r0, n, Bn, An, true);
	}
      }
    } else
      printf("%s", line);
  }

  fclose(inf);
}


// correction algorithms

// control of orbit

// closed orbit correction by n_orbit iterations
bool CorrectCOD(int n_orbit)
{
  bool      cod;
  int       i;
  long int  lastpos;
  Vector2   mean, sigma, max;

  cod = getcod(0.0, lastpos);
  if (cod) {
    codstat(mean, sigma, max, globval.Cell_nLoc, false); // get orbit stats
    printf("\n");
    printf("Initial RMS orbit (BPMs):   x = %7.1e mm, y = %7.1e mm\n",
	   1e3*sigma[X_], 1e3*sigma[Y_]);
    codstat(mean, sigma, max, globval.Cell_nLoc, true);
    printf("Initial RMS orbit (all):    x = %7.1e mm, y = %7.1e mm\n",
	   1e3*sigma[X_], 1e3*sigma[Y_]);

    for (i = 0; i < n_orbit; i++){
      lsoc(1, globval.bpm, globval.hcorr, 1); // correct horizontal orbit
      lsoc(1, globval.bpm, globval.vcorr, 2); // correct vertical orbit
      cod = getcod(0.0, lastpos);             // find closed orbit
      if (!cod) break;
    }

    if (cod) {
      codstat(mean, sigma, max, globval.Cell_nLoc, false);
      printf("Corrected RMS orbit (BPMs): x = %7.1e mm, y = %7.1e mm\n",
	     1e3*sigma[X_], 1e3*sigma[Y_]);
      codstat(mean, sigma, max, globval.Cell_nLoc, true);
      printf("Corrected RMS orbit (all):  x = %7.1e mm, y = %7.1e mm\n",
	     1e3*sigma[X_], 1e3*sigma[Y_]);
    }
  }

  return cod;
}


void Align_BPMs(const int n)
{
  // Align BPMs to adjacent multipoles.

  bool      aligned;
  int       i, j, k;
  long int  loc;

  const int  n_step = 5;

  printf("\n");
  for (i = 1; i <= GetnKid(globval.bpm); i++) {
    loc = Elem_GetPos(globval.bpm, i);

    if ((loc == 1) || (loc == globval.Cell_nLoc)) {
      printf("Align_BPMs: BPM at entrance or exit of lattice: %ld\n", loc);
      exit_(1);
    }

    j = 1; aligned = false;
    do {
      if ((Cell[loc-j].Elem.Pkind == Mpole) &&
	  (Cell[loc-j].Elem.M->n_design == n)) {
	for (k = 0; k <= 1; k++)
	  Cell[loc].Elem.M->PdSsys[k] = Cell[loc-j].dS[k];
	printf("aligned BPM no %1d to %s\n", i, Cell[loc-j].Elem.PName);
	aligned = true; break;
      } else if ((Cell[loc+j].Elem.Pkind == Mpole) &&
		 (Cell[loc+j].Elem.M->n_design == n)) {
	for (k = 0; k <= 1; k++)
	  Cell[loc].Elem.M->PdSsys[k] = Cell[loc+j].dS[k];
	printf("aligned BPM no %1d to %s\n", i, Cell[loc+j].Elem.PName);
	aligned = true; break;
      }

      j++;
    } while (j <= n_step);
      
    if (aligned)
      Mpole_SetdS(globval.bpm, i);
    else
      printf("Align_BPMs: no multipole adjacent to BPM no %d\n", i);
  }
}

/* *****************************************************
 void get_bare()
  
  Purpose:
           store values of the optics function at the sextupoles 
*/
void get_bare()
{
  long int  j, k;

  n_sext = 0;
  for (j = 0; j <= globval.Cell_nLoc; j++) {
    if ((Cell[j].Elem.Pkind == Mpole) && (Cell[j].Elem.M->n_design >= Sext)) {
      n_sext++; sexts[n_sext-1] = j;
      for (k = 0; k <= 1; k++) {
	beta0_[n_sext-1][k] = Cell[j].Beta[k];
	nu0s[n_sext-1][k] = Cell[j].Nu[k];
      }
    }
  }

  // beta-functions for normalization
  beta_ref[X_] = Cell[globval.Cell_nLoc].Beta[X_];
  beta_ref[Y_] = Cell[globval.Cell_nLoc].Beta[Y_];

  nu0_[X_] = globval.TotalTune[X_]; nu0_[Y_] = globval.TotalTune[Y_];
}


void get_dbeta_dnu(double m_dbeta[], double s_dbeta[],
		   double m_dnu[], double s_dnu[])
{
  int       k;
  long int  j, ind;
  double    dbeta, dnu;

  Ring_GetTwiss(false, 0.0);
  
  for (k = 0; k <= 1; k++) {
    m_dbeta[k] = 0.0; s_dbeta[k] = 0.0; m_dnu[k] = 0.0; s_dnu[k] = 0.0;
  }
  
  for (j = 0; j < n_sext; j++) {
    ind = sexts[j];
    for (k = 0; k <= 1; k++) {
      dbeta = (Cell[ind].Beta[k]-beta0_[j][k])/beta0_[j][k];
      m_dbeta[k] += dbeta; s_dbeta[k] += sqr(dbeta);
      dnu = Cell[ind].Nu[k] - nu0s[j][k];
      m_dnu[k] += dnu; s_dnu[k] += sqr(dnu);
    }
  }

  for (k = 0; k <= 1; k++) {
    m_dbeta[k] /= n_sext; m_dnu[k] /= n_sext;
    s_dbeta[k] = sqrt((s_dbeta[k]-n_sext*sqr(m_dbeta[k]))/(n_sext-1));
    s_dnu[k] = sqrt((s_dnu[k]-n_sext*sqr(m_dnu[k]))/(n_sext-1));
  }
}


bool CorrectCOD_N(const char *ae_file, const int n_orbit,
		  const int n, const int k)
{
  bool    cod = false;
  int     i;
  double  m_dbeta[2], s_dbeta[2], m_dnu[2], s_dnu[2];

  // Clear trim setpoints
  set_bnL_design_fam(globval.hcorr, Dip, 0.0, 0.0);
  set_bnL_design_fam(globval.vcorr, Dip, 0.0, 0.0);

  // load misalignments
  LoadAlignTol(ae_file, true, 1.0, true, k);
  for (i = 1; i <= n; i++) {
    // Scale the rms values
    LoadAlignTol(ae_file, true, (double)i/(double)n, false, k);
    
    if (bba) {
      // Beam based alignment
      Align_BPMs(Quad);
    }

    cod = CorrectCOD(n_orbit); 
    
    if (!cod) break;
    
    get_dbeta_dnu(m_dbeta, s_dbeta, m_dnu, s_dnu);
    printf("\n");
    printf("RMS dbeta_x/beta_x = %4.2f%%,   dbeta_y/beta_y = %4.2f%%\n",
	   1e2*s_dbeta[X_], 1e2*s_dbeta[Y_]);
    printf("RMS dnu_x          = %7.5f, dnu_y          = %7.5f\n",
	   s_dnu[X_], s_dnu[Y_]);
  }

  return cod;
}


bool CorrectCoupling_N(const int n)
{
	double deta_y_max = 0;
	ini_skew_cor(deta_y_max);
	corr_eps_y();

}


// Control of vertical beam size

void FindSQ_SVDmat(double **SkewRespMat, double **U, 
		   double **V, double *w, int N_COUPLE, int N_SKEW)
{
  int i, j;

  const double  cut = 1e-10; // cut value for SVD singular values

  for (i = 1; i <= N_COUPLE; i++)
    for (j = 1; j <= N_SKEW; j++)
      U[i][j] = SkewRespMat[i][j];

  // prepare matrices for SVD
  dsvdcmp(U, N_COUPLE, N_SKEW, w, V);

  // zero singular values
  printf("\n");
  printf("singular values:\n");
  printf("\n");
  for (i = 1; i <= N_SKEW; i++) {
    printf("%11.3e", w[i]);
    if (w[i] < cut ) {
      w[i] = 0.0;
      printf(" (zeroed)");
      if (i % 8 == 0) printf("\n");
    }
  }
  if (i % 8 != 0) printf("\n");
}


void FindMatrix(double **SkewRespMat, const double deta_y_max)
{
  //  Ring_GetTwiss(true, 0.0) should be called in advance
  int       i, j, k;
  long int  loc;
  double    nuX, nuY, alpha, eta_y_max;
  double    *etaSQ;
  double    **betaSQ, **nuSQ, **betaBPM, **nuBPM;
  double    **betaHC, **nuHC, **betaVC, **nuVC;
  FILE      *SkewMatFile, *fp;

  const int     Xi = 1, Yi = 2;
  const double  pi = M_PI, twopi = 2.0*M_PI;


  etaSQ = dvector(1, N_SKEW); betaSQ = dmatrix(1, N_SKEW, 1, 2);
  nuSQ = dmatrix(1, N_SKEW, 1, 2);
  betaBPM = dmatrix(1, N_BPM, 1, 2); nuBPM = dmatrix(1, N_BPM, 1, 2);
  betaHC = dmatrix(1, N_HCOR, 1, 2); nuHC = dmatrix(1, N_HCOR, 1, 2);
  betaVC = dmatrix(1, N_VCOR, 1, 2); nuVC = dmatrix(1, N_VCOR, 1, 2);

  nuX = globval.TotalTune[X_]; nuY = globval.TotalTune[Y_];

  for (i = 1; i <= N_SKEW; i++) {
    loc = Elem_GetPos(globval.qt, i);
    etaSQ[i] = Cell[loc].Eta[X_];
    betaSQ[i][Xi] = Cell[loc].Beta[X_]; betaSQ[i][Yi] = Cell[loc].Beta[Y_];
    nuSQ[i][Xi] = Cell[loc].Nu[X_]; nuSQ[i][Yi] = Cell[loc].Nu[Y_];
  } // for i=1..N_SKEW

  for (i = 1; i <= N_BPM; i++) {
    betaBPM[i][Xi] = Cell[bpm_loc[i-1]].Beta[X_];
    betaBPM[i][Yi] = Cell[bpm_loc[i-1]].Beta[Y_];
    nuBPM[i][Xi] = Cell[bpm_loc[i-1]].Nu[X_];
    nuBPM[i][Yi] = Cell[bpm_loc[i-1]].Nu[Y_];
  } // for i=1..N_BPM

  for (i = 1; i <= N_HCOR; i++) {
    betaHC[i][Xi] = Cell[h_corr[i-1]].Beta[X_];
    betaHC[i][Yi] = Cell[h_corr[i-1]].Beta[Y_];
    nuHC[i][Xi] = Cell[h_corr[i-1]].Nu[X_];
    nuHC[i][Yi] = Cell[h_corr[i-1]].Nu[Y_];
  } // for i=1..N_HCOR

  for (i = 1; i <= N_VCOR; i++) {
    betaVC[i][Xi] = Cell[v_corr[i-1]].Beta[X_];
    betaVC[i][Yi] = Cell[v_corr[i-1]].Beta[Y_];
    nuVC[i][Xi] = Cell[v_corr[i-1]].Nu[X_];
    nuVC[i][Yi] = Cell[v_corr[i-1]].Nu[Y_];
  } // for i=1..N_VCOR


  for (i = 1; i <= N_SKEW; i++) {
    // looking for term for vertical dispersion
    alpha = etaSQ[i];
    // printf("For skew quad %3d kick is %9.2e\n",i,alpha);
    for (j = 1; j <= N_BPM; j++) {
      SkewRespMat[j][i] = VDweight*0.5*alpha*sqrt(betaSQ[i][Yi]*betaBPM[j][Yi])
	*cos(twopi*fabs(nuSQ[i][Yi]-nuBPM[j][Yi])-pi*nuY)/sin(pi*nuY);
    } // for (j=1; j<=N_BPM; j++)

    // loking for coupling of horizontal trim to vertical BPM
    for (k = 1; k <= N_HCOR; k++) {
      // find v-kick by i-th skew quad due to the k-th h-trim
      alpha = 0.5*sqrt(betaSQ[i][Xi]*betaHC[k][Xi])*
	cos(twopi*fabs(nuSQ[i][Xi]-nuHC[k][Xi])-pi*nuX)/sin(pi*nuX);
      // find vertical orbit due to the kick
      for (j = 1; j <= N_BPM; j++) 
	SkewRespMat[N_BPM+(k-1)*N_HCOR+j][i] = 
          HVweight*0.5*alpha*sqrt(betaSQ[i][Yi]*betaBPM[j][Yi])*
	  cos(twopi*fabs(nuSQ[i][Yi]-nuBPM[j][Yi])-pi*nuY)/sin(pi*nuY);
    } //for (k=1; k<=N_HCOR; k++)

   //loking for coupling of vertical trim to horizontal BPM
    for (k = 1; k <= N_VCOR; k++) {
      // find h-kick by i-th skew quad due to the k-th v-trim
      alpha = 0.5*sqrt(betaSQ[i][Yi]*betaVC[k][Yi])*
	cos(twopi*fabs(nuSQ[i][Yi]-nuVC[k][Yi])-pi*nuY)/sin(pi*nuY);
      // find horizontal orbit due to the kick
      for (j = 1; j <= N_BPM; j++) 
	SkewRespMat[N_BPM+N_BPM*N_HCOR+(k-1)*N_VCOR+j][i] = 
          VHweight*0.5*alpha*sqrt(betaSQ[i][Xi]*betaBPM[j][Xi])*
	  cos(twopi*fabs(nuSQ[i][Xi]-nuBPM[j][Xi])-pi*nuX)/sin(pi*nuX);
    } //for (k=1; k<=N_VCOR; k++)
  } // for i=1..N_SKEW


  SkewMatFile = file_write(SkewMatFileName);
  for (i = 1; i <= N_SKEW; i++) {
    for (j = 1; j <= N_COUPLE; j++) 
      fprintf(SkewMatFile, "%9.2e ", SkewRespMat[j][i]);
    fprintf(SkewMatFile, "\n");
  }
  fclose(SkewMatFile);

  fp = file_write(deta_y_FileName);
  eta_y_max = 0.0;
  for (j = 1; j <= N_BPM; j++) {
    eta_y[j] = 0.0;
    for (i = 1; i <= N_SKEW; i++)
      if (i % SQ_per_scell == 0) {
	eta_y[j] += 0.5*etaSQ[i]*sqrt(betaSQ[i][Yi]*betaBPM[j][Yi])
	    *cos(twopi*fabs(nuSQ[i][Yi]-nuBPM[j][Yi])-pi*nuY)/sin(pi*nuY);
      }
    eta_y_max = max(fabs(eta_y[j]), eta_y_max);
  }
  for (j = 1; j <= N_BPM; j++)
    eta_y[j] /= eta_y_max;

  for (j = 1; j <= N_BPM; j++) {
    eta_y[j] *= deta_y_max;
    fprintf(fp, "%6.3f %10.3e\n", Cell[bpm_loc[j-1]].S, 1e3*eta_y[j]);
  }
  fclose(fp);

  free_dvector(etaSQ, 1, N_SKEW); free_dmatrix(betaSQ, 1, N_SKEW, 1, 2);
  free_dmatrix(nuSQ, 1, N_SKEW, 1, 2);
  free_dmatrix(betaBPM, 1, N_BPM, 1, 2); free_dmatrix(nuBPM, 1, N_BPM, 1, 2);
  free_dmatrix(betaHC, 1, N_HCOR, 1, 2); free_dmatrix(nuHC, 1, N_HCOR, 1, 2);
  free_dmatrix(betaVC, 1, N_VCOR, 1, 2); free_dmatrix(nuVC, 1, N_VCOR, 1, 2);
} // FindMatrix


void ini_skew_cor(const double deta_y_max)
{
  int  k;

  // No of skew quads, BPMs, and correctors
  N_SKEW = GetnKid(globval.qt);

  N_BPM = 0;
  for (k = 1; k <= GetnKid(globval.bpm); k++) {
    N_BPM++;

    if (N_BPM > max_bpm) {
      printf("ini_skew_cor: max no of BPMs exceeded %d (%d)\n",
	     N_BPM, max_bpm);
      exit_(1);
    }

    bpm_loc[N_BPM-1] = Elem_GetPos(globval.bpm, k);
  }

  N_HCOR = 0;
  h_corr[N_HCOR++] = Elem_GetPos(globval.hcorr, 1);
  h_corr[N_HCOR++] = Elem_GetPos(globval.hcorr, GetnKid(globval.hcorr)/3);
  h_corr[N_HCOR++] = Elem_GetPos(globval.hcorr, 2*GetnKid(globval.hcorr)/3);

  N_VCOR = 0;
  v_corr[N_VCOR++] = Elem_GetPos(globval.vcorr, 1);
  v_corr[N_VCOR++] = Elem_GetPos(globval.vcorr, GetnKid(globval.vcorr)/3);
  v_corr[N_VCOR++] = Elem_GetPos(globval.vcorr, 2*GetnKid(globval.vcorr)/3);

  N_COUPLE = N_BPM*(1+N_HCOR+N_VCOR);

  SkewRespMat = dmatrix(1, N_COUPLE, 1, N_SKEW);
  VertCouple = dvector(1, N_COUPLE);
  SkewStrengthCorr = dvector(1, N_SKEW);
  b = dvector(1, N_COUPLE); w = dvector(1, N_SKEW);
  V = dmatrix(1, N_SKEW, 1, N_SKEW); U = dmatrix(1, N_COUPLE, 1, N_SKEW);
  eta_y = dvector(1, N_BPM);

  printf("\n");
  printf("Number of trims:                   horizontal = %d, vertical = %d\n",
	 N_HCOR, N_VCOR);
  printf("Number of BPMs:                    %6d\n", N_BPM);
  printf("Number of skew quads:              %6d\n", N_SKEW);
  printf("Number of elements in skew vector: %6d\n", N_COUPLE);

  // find matrix
  Ring_GetTwiss(true, 0.0);

  printf("\n");
  printf("Looking for response matrix\n");   
  FindMatrix(SkewRespMat, deta_y_max);

  printf("Looking for SVD matrices\n");  
  FindSQ_SVDmat(SkewRespMat, U, V, w, N_COUPLE, N_SKEW);
}


void FindCoupVector(double *VertCouple)
{
  bool      cod;
  long      i,j;
  long      lastpos;
  double    *orbitP, *orbitN;

  orbitP = dvector(1, N_BPM); orbitN = dvector(1, N_BPM);

  // Find vertical dispersion
  Cell_Geteta(0, globval.Cell_nLoc, true, 0e0);

  for (i = 1; i <= N_BPM; i++)
    VertCouple[i] = VDweight*Cell[bpm_loc[i-1]].Eta[Y_];
  // Finished finding vertical dispersion

  // Find off diagonal terms for horizontal trims
  for (j = 1; j <= N_HCOR; j++) {
    // positive kick: "+Dip" for horizontal
    SetdKLpar(Cell[h_corr[j-1]].Fnum, Cell[h_corr[j-1]].Knum, +Dip, kick);
    cod = getcod(0.0, lastpos); chk_cod(cod, "FindCoupVector");
    for (i = 1; i <= N_BPM; i++)
      orbitP[i] = Cell[bpm_loc[i-1]].BeamPos[y_];

    //negative kick: "+Dip" for horizontal
    SetdKLpar(Cell[h_corr[j-1]].Fnum, Cell[h_corr[j-1]].Knum, +Dip, -2*kick);
    cod = getcod(0.0, lastpos); chk_cod(cod, "FindCoupVector");
    for (i = 1; i <= N_BPM; i++)
      orbitN[i] = Cell[bpm_loc[i-1]].BeamPos[y_];

    // restore trim valueL: "+Dip" for horizontal
    SetdKLpar(Cell[h_corr[j-1]].Fnum, Cell[h_corr[j-1]].Knum, +Dip, kick);

    for (i = 1; i <= N_BPM; i++)
      VertCouple[N_BPM+(j-1)*N_HCOR+i] = 
	HVweight*(orbitN[i]-orbitP[i])*0.5/kick; // sign reversal
  } // hcorr cycle


  // Find off diagonal terms for vertical trims
  for (j = 1; j <= N_VCOR; j++){
    // positive kick: "-Dip" for vertical
    SetdKLpar(Cell[v_corr[j-1]].Fnum, Cell[v_corr[j-1]].Knum, -Dip, kick);
    cod = getcod(0.0, lastpos); chk_cod(cod, "FindCoupVector");
    for (i = 1;  i <= N_BPM; i++)
      orbitP[i] = Cell[bpm_loc[i-1]].BeamPos[x_];

    // negative kick: "-Dip" for vertical
    SetdKLpar(Cell[v_corr[j-1]].Fnum, Cell[v_corr[j-1]].Knum, -Dip, -2*kick);
    cod = getcod(0.0, lastpos); chk_cod(cod, "FindCoupVector");
    for (i = 1; i <= N_BPM; i++)
      orbitN[i] = Cell[bpm_loc[i-1]].BeamPos[x_];

    // restore corrector: "-Dip" for vertical
    SetdKLpar(Cell[v_corr[j-1]].Fnum, Cell[v_corr[j-1]].Knum, -Dip, kick);

    for (i = 1; i <= N_BPM; i++) 
      VertCouple[N_BPM+N_BPM*N_HCOR+(j-1)*N_VCOR+i] = 
	VHweight*(orbitP[i]-orbitN[i])*0.5/kick;
  } // vcorr cycle

  free_dvector(orbitP, 1, N_BPM); free_dvector(orbitN, 1, N_BPM);
} // FindCoupVector


void SkewStat(double VertCouple[])
{
  int     i;
  double  max, rms, sk;

  // statistics for skew quadrupoles
  max = 0.0; rms = 0.0;
  for(i = 1; i <= N_SKEW; i++) {
    sk = GetKLpar(globval.qt, i, -Quad);
    if (fabs(sk) > max) max = fabs(sk);
    rms += sqr(sk);
  }
  rms = sqrt(rms/N_SKEW);
  printf("Rms skew strength:       %8.2e+/-%8.2e\n", max, rms);

  // statistics for vertical dispersion function
  max = 0.0; rms = 0.0;
  for(i = 1; i <= N_BPM; i++) {
    if (fabs(VertCouple[i]) > max) max = fabs(VertCouple[i]/VDweight);
    rms += sqr(VertCouple[i]/VDweight);
  }
  rms = sqrt(rms/N_BPM);
  printf("Max vertical dispersion: %8.2e+/-%8.2e mm\n", 1e3*max, 1e3*rms);

  // statistics for off diagonal terms of response matrix (trims->bpms)
  max = 0.0; rms = 0.0;
  for(i = N_BPM+1; i <= N_BPM*(1+N_HCOR); i++) {
    if (fabs(VertCouple[i]) > max) max = fabs(VertCouple[i]/HVweight);
    rms += sqr(VertCouple[i]/HVweight);
  }
  rms = sqrt(rms/(N_HCOR*N_BPM));
  printf("Max horizontal coupling: %8.2e+/-%8.2e mm/mrad\n", max, rms);

  max = 0.0; rms = 0.0;
  for(i = N_BPM*(1+N_HCOR)+1; i <= N_COUPLE; i++) {
    if (fabs(VertCouple[i]) > max) max = fabs(VertCouple[i]/VHweight);
    rms += sqr(VertCouple[i]/VHweight);
  }
  rms = sqrt(rms/(N_VCOR*N_BPM));
  printf("Max vertical coupling:   %8.2e+/-%8.2e mm/mrad\n", max, rms);
}


void corr_eps_y(void)
{
  int   i, j;
  FILE  *outf;

  // Clear skew quad setpoints
  set_bnL_design_fam(globval.qt, Quad, 0.0, 0.0);

  // Find coupling vector
  printf("\n");
  printf("Looking for coupling error\n");
  FindCoupVector(VertCouple);

  //Find and print coupling statistics
  printf("\n");
  printf("Before correction\n");
  SkewStat(VertCouple);

  // Coupling Correction
  printf("\n");
  for (i = 1; i <= n_lin; i++) {
    printf("Looking for correction\n");

    //Find Correcting Settings to skew quadrupoles
    for (j = 1; j <= N_BPM; j++)
      b[j] = VDweight*eta_y[j] - VertCouple[j];

    for (j = N_BPM+1; j <= N_COUPLE; j++)
      b[j] = -VertCouple[j];

    dsvbksb(U, w, V, N_COUPLE, N_SKEW, b, SkewStrengthCorr);

    printf("Applying correction\n");
    // Add correction
    for (j = 1; j <= N_SKEW; j++) 
      SetdKLpar(globval.qt, j, -Quad, SkewStrengthCorr[j]);

    printf("\n");
    printf("Looking for coupling error\n");
    // Find coupling vector
    FindCoupVector(VertCouple);

    printf("\n");
    printf("After run %d of correction\n", i);
    // Find and print coupling statistics
    SkewStat(VertCouple);
  } // End of coupling Correction

  outf = file_write(eta_y_FileName);
  for (i = 0; i <= globval.Cell_nLoc; i++)
    fprintf(outf, "%4d %7.3f %s %6.3f %10.3e %10.3e\n",
	    i, Cell[i].S, Cell[i].Elem.PName,
	    Cell[i].Nu[Y_], 1e3*Cell[i].Eta[Y_], 1e3*Cell[i].Etap[Y_]);
  fclose(outf);

  FindCoupVector(VertCouple);
}


// Control of IDs

void get_IDs(void)
{
  int  k;

  printf("\n");
  n_ID_Fams = 0;
  for (k = 0; k < globval.Elem_nFam; k++)
    switch (ElemFam[k].ElemF.Pkind) {
    case Wigl:
      printf("found ID family:   %s %12.5e\n",
	     ElemFam[k].ElemF.PName, ElemFam[k].ElemF.W->BoBrhoV[0]);
      n_ID_Fams++; ID_Fams[n_ID_Fams-1] = k + 1;
      break;
    case Insertion:
      printf("found ID family:   %s %12.5e\n",
	     ElemFam[k].ElemF.PName, ElemFam[k].ElemF.ID->scaling1);
      n_ID_Fams++; ID_Fams[n_ID_Fams-1] = k + 1;
      break;
    case FieldMap:
      printf("found ID family:   %s %12.5e\n",
	     ElemFam[k].ElemF.PName, ElemFam[k].ElemF.FM->scl);
      n_ID_Fams++; ID_Fams[n_ID_Fams-1] = k + 1;
      break;
    default:
      break;
    }
}

/****************************************************************************/
/* void set_IDs(const double scl)

   Purpose:
       Set the scale factor for all the wigger, insertion devices, or fieldMap.        

   Input:
       scl   scaling factor

   Output:
      none

   Return:
       none

   Global variables:
       none

   Specific functions:
       none

   Comments:
       See also:
                 set_ID_scl(const int Fnum, const double scl)
		 set_ID_scl(const int Fnum, const int knum, const double scl)
       
****************************************************************************/
void set_IDs(const double scl)
{
  int  k;

  printf("\n");
  for (k = 0; k < n_ID_Fams; k++) {
    switch (ElemFam[ID_Fams[k]-1].ElemF.Pkind) {
    case Wigl:
      printf("setting ID family: %s %12.5e\n",
	     ElemFam[ID_Fams[k]-1].ElemF.PName,
	     scl*ElemFam[ID_Fams[k]-1].ElemF.W->BoBrhoV[0]);

      set_Wiggler_BoBrho(ID_Fams[k],
			 scl*ElemFam[ID_Fams[k]-1].ElemF.W->BoBrhoV[0]);
      break;
    case Insertion:
      printf("setting ID family: %s %12.5e\n",
	     ElemFam[ID_Fams[k]-1].ElemF.PName, scl);

      set_ID_scl(ID_Fams[k], scl);
      break;
    case FieldMap:
      printf("setting ID family: %s %12.5e\n",
	     ElemFam[ID_Fams[k]-1].ElemF.PName, scl);

      set_ID_scl(ID_Fams[k], scl);
      break;
    default:
      cout << "set_IDs: unknown element type" << endl;
      exit_(1);
      break;
    }
  }
}


void reset_quads(void)
{
  int       k;

  if (N_Fam > N_Fam_max) {
    printf("reset_quads: N_Fam > N_Fam_max: %d (%d)\n", N_Fam, N_Fam_max);
    exit_(0);
  }

  for (k = 0; k < N_Fam; k++) {
    // Note, actual values can differ from the original values
/*    printf("setting quad family: %s %12.5e\n",
	   ElemFam[Q_Fam[k]-1].ElemF.PName,
	   ElemFam[Q_Fam[k]-1].ElemF.M->PBpar[HOMmax+Quad]);

    set_bn_design_fam(Q_Fam[k], Quad,
		       ElemFam[Q_Fam[k]-1].ElemF.M->PBpar[HOMmax+Quad], 0.0);*/

    printf("setting quad family: %s %12.5e\n",
	   ElemFam[Q_Fam[k]-1].ElemF.PName, b2[k]);

    set_bn_design_fam(Q_Fam[k], Quad, b2[k], 0.0);
  }
}


void SVD(const int m, const int n, double **M, double beta_nu[],
	 double b2Ls_[], const double s_cut, const bool first)
{
  int     i, j;

  const bool    prt   = true;

  if (first) {
    for (i = 1; i <= m; i++)
      for (j = 1; j <= n; j++)
	U1[i][j] = M[i][j];

    dsvdcmp(U1, m, n, w1, V1);

    if (prt) { 
      printf("\n");
      printf("singular values:\n");
      printf("\n");
    }

    for (i = 1; i <= n; i++) {
      if (prt) printf("%11.3e", w1[i]);
      if (w1[i] < s_cut) {
	w1[i] = 0.0;
	if (prt) printf(" (zeroed)");
      }
      if (prt) if (i % 8 == 0) printf("\n");
    }
    if (prt) if (n % 8 != 0) printf("\n");
  }
 
  dsvbksb(U1, w1, V1, m, n, beta_nu, b2Ls_);
}


void quad_config()
{
  int     i, j;
  double  an;

  if (N_Fam > N_Fam_max) {
    printf("quad_config: N_Fam > N_Fam_max: %d (%d)\n", N_Fam, N_Fam_max);
    exit_(0);
  }

  Nquad = 0;
  for (i = 0; i < N_Fam; i++) {
    for (j = 1; j <= GetnKid(Q_Fam[i]); j++) {
      Nquad++;

      if (Nquad > n_b2_max) {
        printf("quad_config: max no of quadrupoles exceeded %d (%d)\n",
               Nquad, n_b2_max);
        exit_(1);
      }

      quad_prms[Nquad-1] = Elem_GetPos(Q_Fam[i], j);

      if (j == 1) get_bn_design_elem(Q_Fam[i], j, Quad, b2[i], an);
    }
  }

  printf("\n");
  printf("quad_config: Nquad = %d\n", Nquad);
}


bool get_SQ(void)
{
  int      j, k;
  Vector2  alpha3[3], beta3[3], nu3[3], eta3[3], etap3[3];
  FILE     *outf;

  const bool  prt = false;
 
  /* Note, IDs are split for evaluation of the driving terms at the center:
       id1  1, 2
       id2  1, 2
       ...                                                                  */

  // Get Twiss params, no dispersion
  Ring_GetTwiss(false, 0.0);

  if (!status.codflag || !globval.stable) return false;

  // Get global tunes
  Nu_X = globval.TotalTune[X_]; Nu_Y = globval.TotalTune[Y_];

  if (prt) {
    printf("\n");
    printf("nu_x = %8.12f, nu_y = %8.12f\n", Nu_X, Nu_Y);

    // Get Twiss params in sext
    printf("\n");
    printf("listing lattice functions at sextupoles\n");
    printf("\n");

    outf = file_write("latfunS.out");

    fprintf(outf, "s betax nux betay nuy\n");
  }

  printf("\n");
  Nsext = 0;
  for (k = 0; k < globval.Cell_nLoc; k++) {
    if ((Cell[k].Elem.Pkind == Mpole) && (Cell[k].Elem.M->n_design == Sext)) {
      Nsext++;

      if (Nsext > n_b3_max) {
        printf("get_SQ: max no of sextupoles exceeded %d (%d)\n",
               Nsext, n_b3_max);
        exit_(1);
      }

      Ss[Nsext-1] = Cell[k].S;

      for (j = 0; j <= 1; j++) {
	sb[j][Nsext-1] = Cell[k].Beta[j];
	sNu[j][Nsext-1] = Cell[k].Nu[j] - nu_0[j];
      }

      if (prt) {
	printf("%s %6.3f %8.5f %8.5f %8.5f %8.5f\n",
	       Cell[k].Elem.PName, Ss[Nsext-1],
	       sb[X_][Nsext-1], sNu[X_][Nsext-1]-nu_0[X_],
	       sb[Y_][Nsext-1], sNu[Y_][Nsext-1]-nu_0[Y_]);
	fprintf(outf, "%s %6.3f %8.5f %8.5f %8.5f %8.5f\n",
		Cell[k].Elem.PName, Ss[Nsext-1],
		sb[X_][Nsext-1], sNu[X_][Nsext-1]-nu_0[X_],
		sb[Y_][Nsext-1], sNu[Y_][Nsext-1]-nu_0[Y_]);
      }
    }
  }

  if (prt) fclose(outf);

  // Number of sexts in the ring
  printf("No of sextupoles = %d\n", Nsext);

  if (prt) {
    // Get Twiss params in quads
    printf("\n");
    printf("listing lattice functions at quadrupoles\n");
    printf("\n");

    outf = file_write("latfunQ.out");

    fprintf(outf, "s name betax nux betay nuy\n");
  }

  for (k = 0; k < Nquad; k++) {
    Sq[k] = Cell[quad_prms[k]].S;
    for (j = 0; j <= 1; j++) {
      if (Cell[quad_prms[k]].Elem.M->Pthick == thick) {
	get_twiss3(quad_prms[k], alpha3, beta3, nu3, eta3, etap3);
	qb[j][k] = beta3[Y_][j]; qNu[j][k] = nu3[Y_][j] - nu_0[j];
      } else {
	qb[j][k] = Cell[quad_prms[k]].Beta[j];
	qNu[j][k] = Cell[quad_prms[k]].Nu[j] - nu_0[j];
      }
    }

    if (prt) {
      printf("%s %6.3f %8.5f %8.5f %8.5f %8.5f\n",
	     Cell[quad_prms[k]].Elem.PName, Sq[k], qb[X_][k],
	     qNu[X_][k], qb[Y_][k], qNu[Y_][k]);

      fprintf(outf, "%s %6.3f %8.5f %8.5f %8.5f %8.5f\n",
	      Cell[quad_prms[k]].Elem.PName, Sq[k], qb[X_][k],
	      qNu[X_][k], qb[Y_][k], qNu[Y_][k]);
    }
  }

  if (prt) fclose(outf);

  // Number of quads in the ring
  printf("No of quads      = %d\n", Nquad);

  return true;
}


double Bet(double bq, double nus, double nuq, double NuQ)
{
  return bq*cos(2.0*M_PI*(2.0*fabs(nus-nuq)-NuQ))/(2.0*sin(2.0*M_PI*NuQ));
}


double Nus(double bq, double nus, double nuq, double NuQ)
{
  double  Nu, sgn;

  sgn = ((nus-nuq) <= 0)? -1: 1;

  Nu = -bq*sgn*(sin(2.0*M_PI*NuQ)+sin(2.0*M_PI*(2.0*fabs(nus-nuq)-NuQ)))
       /(8.0*M_PI*sin(2.0*M_PI*NuQ));

  return Nu;
}


void A_matrix(void)
{
  int     k, j;
  double  BtX, BtY, NuX, NuY;

  const bool  prt = false;

  // Defining Twiss in undisturbed quads
  for (k = 0; k < Nquad; k++)
    for (j = 0; j <= 1; j++) {
      qb0[j][k] = qb[j][k]; 
      qNu0[j][k] = qNu[j][k];
    }
  
  // Defining Twiss in undisturbed sexts
  for (k = 0; k < Nsext; k++)
    for (j = 0; j <= 1; j++)
      sNu0[j][k] = sNu[j][k];
  
  // Now creating matrix A in X=A*B2L
  for (k = 1; k <= Nsext; k++) {
    for (j = 1; j <= Nquad; j++) {
      BtX = Bet(qb0[X_][j-1], sNu0[X_][k-1], qNu0[X_][j-1], Nu_X0);
      NuX = -Nus(qb0[X_][j-1], sNu0[X_][k-1], qNu0[X_][j-1], Nu_X0);
      BtY = -Bet(qb0[Y_][j-1], sNu0[Y_][k-1], qNu0[Y_][j-1], Nu_Y0);
      NuY = Nus(qb0[Y_][j-1], sNu0[Y_][k-1], qNu0[Y_][j-1], Nu_Y0);
      A1[k][j] = scl_dbeta*BtX;
      A1[k+Nsext][j] = scl_dbeta*BtY;
      A1[k+2*Nsext][j] = scl_dnu*NuX;
      A1[k+3*Nsext][j] = scl_dnu*NuY;
    }
  }
  // Now adding 2 more constraints for global tunes
  for (j = 1; j <= Nquad; j++) {
    A1[4*Nsext+1][j] = -scl_nu*qb0[X_][j-1]/(4.0*M_PI);
    A1[4*Nsext+2][j] =  scl_nu*qb0[Y_][j-1]/(4.0*M_PI);
  }

  if (prt) {
    printf("\n");
    printf("AA:\n");
    printf("\n");
    for (k = 1; k <= Nconstr; k++) {
      for (j = 1; j <= Nquad; j++)
	printf(" %10.3e", A1[k][j]);
      printf("\n");
    }
  }
}


void X_vector(const bool first)
{
  int  k;

  const bool  prt = false;

  dnu0[X_] = globval.TotalTune[X_] - Nu_X0;
  dnu0[Y_] = globval.TotalTune[Y_] - Nu_Y0;

  if (first) {
    // Initial fill of X
    for (k = 1; k <= Nsext; k++) {
      Xsext0[k]         = sb[X_][k-1];  Xsext0[k+Nsext]   = sb[Y_][k-1];
      Xsext0[k+2*Nsext] = sNu[X_][k-1]; Xsext0[k+3*Nsext] = sNu[Y_][k-1];
    }
    Xsext0[4*Nsext+1] = 0.0; Xsext0[4*Nsext+2] = 0.0;
  } else { 
    // Now substracting from X in X=A*B2L 
    for (k = 1; k <= Nsext; k++) {
      Xsext[k]         = scl_dbeta*(Xsext0[k]-sb[X_][k-1])/sb[X_][k-1];
      Xsext[k+Nsext]   = scl_dbeta*(Xsext0[k+Nsext]-sb[Y_][k-1])/sb[Y_][k-1];
      Xsext[k+2*Nsext] = scl_dnu*(Xsext0[k+2*Nsext]-sNu[X_][k-1]+dnu0[X_]/2.0);
      Xsext[k+3*Nsext] = scl_dnu*(Xsext0[k+3*Nsext]-sNu[Y_][k-1]+dnu0[Y_]/2.0);
    }
    Xsext[4*Nsext+1] = scl_nu*(Nu_X0-globval.TotalTune[X_]);
    Xsext[4*Nsext+2] = scl_nu*(Nu_Y0-globval.TotalTune[Y_]);
  }

  if (prt) {
    printf("\n");
    printf("X:\n");
    printf("\n");
    if (first) {
      for (k = 1; k <= Nconstr; k++) {
	printf(" %10.3e", Xsext0[k]);
	if (k % 10 == 0)  printf("\n");
      }
      if (Nconstr % 10 != 0) printf("\n");
    } else {
      for (k = 1; k <= Nconstr; k++) {
	printf(" %10.3e", Xsext[k]);
	if (k % 10 == 0)  printf("\n");
      }
      if (Nconstr % 10 != 0) printf("\n");
    }
  }
}


void ini_ID_corr(void)
{

  // store ID families
  get_IDs();

  // zero ID's
  set_IDs(0.0);

  // Configuring quads (1 --> C means thin quads located in the middle of 1s)
  quad_config();

  // Configuring quads (1 --> C means thin quads located in the middle of 1s)
  // Read Betas and Nus
  get_SQ(); 
  Nconstr = 4*Nsext + 2;

  // Note, allocated vectors and matrices are deallocated in ID_corr
  Xsext = dvector(1, Nconstr); Xsext0 = dvector(1, Nconstr);
  b2Ls_ = dvector(1, Nquad); A1 = dmatrix(1, Nconstr, 1, Nquad);
  U1 = dmatrix(1, Nconstr, 1, Nquad); w1 = dvector(1, Nquad);
  V1 = dmatrix(1, Nquad, 1, Nquad);

  // shift zero point to center of ID
//  nu_0[X_] = Cell[id_loc].Nu[X_]; nu_0[Y_] = Cell[id_loc].Nu[Y_];
  nu_0[X_] = 0.0; nu_0[Y_] = 0.0;

  // Defining undisturbed tunes
  Nu_X0 = globval.TotalTune[X_]; Nu_Y0 = globval.TotalTune[Y_];

  // Set-up matrix A in X=A*b2Ls_
  A_matrix();

  // Now fill the X in X=A*b2Ls_ 
  X_vector(true);
}


void W_diag(void)
{
  double    bxf, byf, nxf, nyf, b2Lsum;
  int       k;

  bxf = 0.0; byf = 0.0; nxf = 0.0; nyf = 0.0;
  for (k = 1; k <= Nsext; k++) {
    bxf += sqr(Xsext[k]);
    byf += sqr(Xsext[k+Nsext]);
    nxf += sqr(Xsext[k+2*Nsext]);
    nyf += sqr(Xsext[k+3*Nsext]);
  }

  dnu0[X_] = globval.TotalTune[X_] - Nu_X0;
  dnu0[Y_] = globval.TotalTune[Y_] - Nu_Y0;

  b2Lsum = 0.0;
  for (k = 1; k <= Nquad; k++) 
    b2Lsum += sqr(b2Ls_[k]);

  printf("\n");
  printf("Residuals: beta [%%], dnu : \n");
  printf("dbeta_x: %6.2f dbeta_y: %6.2f nu_x: %12.6e nu_y: %12.6e\n",
	 sqrt(bxf)/Nsext*1e2, sqrt(byf)/Nsext*1e2,
	 sqrt(nxf)/Nsext, sqrt(nyf)/Nsext);
  printf("tune shift: dnu_x = %8.5f, dnu_y = %8.5f\n", dnu0[X_], dnu0[Y_]);
  printf("Sum b2Ls_: %12.6e\n", sqrt(b2Lsum)/Nquad);
}


bool ID_corr(const int N_calls, const int N_steps)
{
  int     i, j, k;
  double  b2, a2;
  FILE    *outf;

  printf("\n");
  printf("ID matching begins!\n");

  outf = file_write("ID_corr.out");
  for (i = 1; i <= N_steps; i++) { //This brings ID strength in steps
    set_IDs((double)i/(double)N_steps);

    get_SQ();                               // Read Betas and Nus
    X_vector(false);                        // Fill in dX in dX=A*db2Ls_ 
    W_diag();                               // Get statistics
    for (j = 1; j <= N_calls; j++) {
      SVD(Nconstr, Nquad, A1, Xsext, b2Ls_, 1e0, j == 1);

      if ((i == N_steps) && (j == N_calls)) {
	fprintf(outf, "b_2:\n");
	fprintf(outf, "\n");
      }

      // add quad strengths (db2Ls_)
      for (k = 1; k <= Nquad; k++) {
	set_dbnL_design_elem(Cell[quad_prms[k-1]].Fnum,
			     Cell[quad_prms[k-1]].Knum, Quad,
			     -ID_step*b2Ls_[k], 0.0);

	if ((i == N_steps) && (j == N_calls)) {
	  get_bn_design_elem(Cell[quad_prms[k-1]].Fnum,
			     Cell[quad_prms[k-1]].Knum, Quad, b2, a2);
	  fprintf(outf, "%10s %2d %8.5f\n",
		  Cell[quad_prms[k-1]].Elem.PName, k, b2);
	}
      }

      printf("\n");
      printf("Iteration: %2d\n", j);
      if (get_SQ()) {
	X_vector(false);                    // Fill in dX in dX=A*db2Ls_ 
	W_diag();                           // Get statistics

	printglob();
      } else {
	printf("ID_corr: correction failed\n");
	// restore lattice
	set_IDs(0.0); reset_quads();
	return false;
      }
    }
  }
  fclose(outf);

  outf = file_write("ID_corr_res.out");
  fprintf(outf, "# dbeta_x/beta_x  dbeta_y/beta_y  dnu_x  dnu_y\n");
  fprintf(outf, "#      [%%]             [%%]\n");
  fprintf(outf, "#\n");
  for (k = 1; k <= Nsext; k++)
    fprintf(outf, "%6.1f %6.2f %6.2f %10.3e %10.3e\n",
	    Ss[k-1], 1e2*Xsext[k], 1e2*Xsext[k+Nsext],
	    Xsext[k+2*Nsext], Xsext[k+3*Nsext]);
  fclose(outf);

  // Allow for repeated calls to ID_corr, allocation is done in ini_ID_corr.
  if (false) {
    free_dvector(Xsext, 1, Nconstr); free_dvector(Xsext0, 1, Nconstr);
    free_dvector(b2Ls_, 1, Nquad); free_dmatrix(A1, 1, Nconstr, 1, Nquad);
    free_dmatrix(U1, 1, Nconstr, 1, Nquad); free_dvector(w1, 1, Nquad);
    free_dmatrix(V1, 1, Nquad, 1, Nquad);
  }

  printf("\n");
  printf("ID matching ends!\n");

  return true;
}

/*  End ID correction functions *****/


void get_param(const char *param_file)
/*
 * Read parameter file in input file xxxx.prm
 *
 */
{
  char    line[max_str], name[max_str], str[max_str], s_prm[max_str];
  char    lat_file[max_str], flat_file[max_str], IDCq_name[max_str][8];
  int     k;
  double  f_prm;
  FILE    *inf;

  const bool  prt = true;

  if (prt) {
    printf("\n");
    printf("reading in %s\n", param_file);
  }

  inf = file_read(param_file);

  // read parameter file
  // initialization
  strcpy(ae_file, ""); strcpy(fe_file, ""); strcpy(ap_file, "");

  if (prt) printf("\n");
  // loop over all lines of the file
  while (fgets(line, max_str, inf) != NULL) {
    if (prt) printf("%s", line);
    if (strstr(line, "#") == NULL) {
      // get initial command token
      sscanf(line, "%s", name);
        // input files *************************************
      if (strcmp("in_dir", name) == 0)
	sscanf(line, "%*s %s", in_dir);
      else if (strcmp("ae_file", name) == 0){ 
	sscanf(line, "%*s %s", str);
	sprintf(ae_file,"%s/%s", in_dir, str);
      } else if (strcmp("fe_file", name) == 0) { 
	sscanf(line, "%*s %s", str);
        sprintf(fe_file, "%s/%s", in_dir, str);
      } else if (strcmp("ap_file", name) == 0) {
	sscanf(line, "%*s %s", str);
	sprintf(ap_file, "%s/%s", in_dir, str);
      } else if (strcmp("lat_file", name) == 0) {
	sscanf(line, "%*s %s", lat_file);
	sprintf(lat_FileName, "%s/%s", in_dir, lat_file);
	Read_Lattice(lat_FileName);
      } else if (strcmp("flat_file", name) == 0) {
	sscanf(line, "%*s %s", flat_file);
	strcat(flat_file, "flat_file.dat"); rdmfile(flat_file);
      // **********< parameters >****************************
      } else if (strcmp("s_cut", name) == 0) {
	sscanf(line, "%*s %lf", &f_prm);
	setrancut(f_prm);
      } else if (strcmp("n_stat", name) == 0)
	sscanf(line, "%*s %d", &n_stat);
      else if (strcmp("n_scale", name) == 0)
	sscanf(line, "%*s %d", &n_scale);
      else if (strcmp("n_orbit", name) == 0)
	sscanf(line, "%*s %d", &n_orbit);
      else if (strcmp("bpm_name", name) == 0) {
	sscanf(line, "%*s %s", s_prm);
	globval.bpm = ElemIndex(s_prm);
      } else if (strcmp("h_corr", name) == 0) {
	sscanf(line, "%*s %s", s_prm);
	globval.hcorr = ElemIndex(s_prm);
      } else if (strcmp("v_corr", name) == 0) {
	sscanf(line, "%*s %s", s_prm);
	globval.vcorr = ElemIndex(s_prm);
      } else if (strcmp("gs", name) == 0) {
	sscanf(line, "%*s %s", s_prm);
	globval.gs = ElemIndex(s_prm);
      } else if (strcmp("ge", name) == 0) {
	sscanf(line, "%*s %s", s_prm);
	globval.ge = ElemIndex(s_prm);
      } else if (strcmp("DA_bare", name) == 0) {
	sscanf(line, "%*s %s", s_prm);
	DA_bare = (strcmp(s_prm, "true") == 0)? true : false;
      } else if (strcmp("delta", name) == 0) { 
	sscanf(line, "%*s %lf", &delta_DA_);
      } else if (strcmp("freq_map", name) == 0) {
	sscanf(line, "%*s %s %d %d %d %d %lf %lf %lf",
	       s_prm, &n_x, &n_y, &n_dp, &n_tr,
	       &x_max_FMA, &y_max_FMA, &delta_FMA);
	freq_map = (strcmp(s_prm, "true") == 0)? true : false;
      } else if (strcmp("tune_shift", name) == 0) {
	sscanf(line, "%*s %s", s_prm);
	tune_shift = (strcmp(s_prm, "true") == 0)? true : false;
      } else if (strcmp("qt", name) == 0) {
	sscanf(line, "%*s %s", s_prm);
	globval.qt = ElemIndex(s_prm);
      } else if (strcmp("disp_wave_y", name) == 0) 
	sscanf(line, "%*s %lf", &disp_wave_y);
      else if (strcmp("n_lin", name) == 0)
	sscanf(line, "%*s %d", &n_lin);
      else if (strcmp("VDweight", name) == 0) 
	sscanf(line, "%*s %lf", &VDweight);
      else if (strcmp("HVweight", name) == 0) 
	sscanf(line, "%*s %lf", &HVweight);
      else if (strcmp("VHweight", name) == 0)
	sscanf(line, "%*s %lf", &VHweight);
      else if (strcmp("N_calls", name) == 0) // ID correction parameters
	sscanf(line, "%*s %d", &N_calls);
      else if (strcmp("N_steps", name) == 0)
	sscanf(line, "%*s %d", &N_steps);
      else if (strcmp("N_Fam", name) == 0)
	sscanf(line, "%*s %d", &N_Fam);
      else if (strcmp("IDCquads", name) == 0) {
	sscanf(line, "%*s %s %s %s %s %s %s %s %s",
	       IDCq_name[0], IDCq_name[1], IDCq_name[2], IDCq_name[3],
	       IDCq_name[4], IDCq_name[5], IDCq_name[6], IDCq_name[7]);
//      } else if (strcmp("scl_nu", name) == 0)
//	sscanf(line, "%*s %lf", &scl_nu);
      } else {
	printf("bad line in %s: %s\n", param_file, line);
        exit_(1);
      }
    } else
      printf("%s", line);
  }

  fclose(inf);

  if (N_calls > 0) {
    if (N_Fam > N_Fam_max) {
      printf("get_param: N_Fam > N_Fam_max: %d (%d)\n",
	     N_Fam, N_Fam_max);
      exit_(0);
    }

    for (k = 0; k < N_Fam; k++)
      Q_Fam[k] = ElemIndex(IDCq_name[k]);
  }
}


// control of vertical beam size

// output procedures

// Finding statistics on orbit in the sextupoles and maximal trim settings
void Orb_and_Trim_Stat(void)
{
  int     i, j, N;
  int     SextCounter = 0;
  int     bins[5] = { 0, 0, 0, 0, 0 };
  double  bin = 40.0e-6; // bin size for stat
  double  tr; // trim strength

  Vector2   Sext_max, Sext_sigma, TrimMax, orb;
  
  Sext_max[X_] = 0.0; Sext_max[Y_] = 0.0; 
  Sext_sigma[X_] = 0.0; Sext_sigma[Y_] = 0.0;
  TrimMax[X_] = 0.0; TrimMax[Y_] = 0.0;
  N = globval.Cell_nLoc; SextCounter = 0;
  for (i = 0; i <= N; i++) {
    if ((Cell[i].Elem.Pkind == Mpole) && (Cell[i].Elem.M->n_design == Sext)) {
      SextCounter++;
      orb[X_] = Cell[i].BeamPos[x_]; orb[Y_] = Cell[i].BeamPos[y_];
      Sext_sigma[X_] += orb[X_]*orb[X_]; Sext_sigma[Y_] += orb[Y_]*orb[Y_];
      if (fabs(orb[X_]) > Sext_max[X_]) Sext_max[X_] = fabs(orb[X_]);
      if (fabs(orb[Y_]) > Sext_max[Y_]) Sext_max[Y_] = fabs(orb[Y_]);
      j = (int) (sqrt(orb[X_]*orb[X_]+orb[Y_]*orb[Y_])/bin);
      if (j > 4) j = 4;
      bins[j]++;
    } // sextupole handling

    if (Cell[i].Fnum == globval.hcorr) {
      tr = Cell[i].Elem.M->PBpar[HOMmax+Dip];
      if (fabs(tr) > TrimMax[X_]) TrimMax[X_] = fabs(tr);
    } // horizontal trim handling
    if (Cell[i].Fnum == globval.vcorr) {
      tr = Cell[i].Elem.M->PBpar[HOMmax-Dip];
      if (fabs(tr) > TrimMax[Y_]) TrimMax[Y_] = fabs(tr);
    } // vertical trim handling
  } // looking throught the cells

  Sext_sigma[X_] = sqrt(Sext_sigma[X_]/SextCounter);
  Sext_sigma[Y_] = sqrt(Sext_sigma[Y_]/SextCounter);
    
  printf("In sextupoles maximal horizontal orbit is:"
	 " %5.3f mm with sigma %5.3f mm\n",
          1e3*Sext_max[X_], 1e3*Sext_sigma[X_]);
  printf("and maximal vertical orbit is:            "
	 " %5.3f mm with sigma %5.3f mm.\n",
	 1e3*Sext_max[Y_], 1e3*Sext_sigma[Y_]);

  for (i = 0; i < 4;  i++) {
    printf("There are %3d sextupoles with offset between ", bins[i]);
    printf(" %5.3f mm and %5.3f mm\n", i*bin*1e3, (i+1)*bin*1e3);
  }
  printf("There are %3d sextupoles with offset ", bins[4]);
  printf("more than %5.3f mm \n", 4e3*bin);
  printf("Maximal hcorr is %6.3f mrad, maximal vcorr is %6.3f mrad\n",
	 1e3*TrimMax[X_], 1e3*TrimMax[Y_]);
}


void prt_codcor_lat(void)
{
  int   i;
  FILE  *CodCorLatFile;


  CodCorLatFile = file_write(CodCorLatFileName);

  fprintf(CodCorLatFile, "#    name     s   sqrt(BxBy) betaX    nuX"
	  "    betaY    nuY     etaX etaX*betaY nuX-nuY \n");
  fprintf(CodCorLatFile, "#            [m]     [m]      [m]             [m]"
	  "              [m]     [m*m] \n");

  for (i = 0; i <= globval.Cell_nLoc; i++){
    fprintf(CodCorLatFile, "%4d:", i);

    if (i == 0)
      fprintf(CodCorLatFile, "%.*s", 6, "begin ");
    else
      fprintf(CodCorLatFile, "%.*s", 6, Cell[i].Elem.PName);

    fprintf(CodCorLatFile, "%7.3f  %5.2f    %5.2f  %7.4f  %5.2f  %7.4f"
	    "  %6.3f  %6.3f  %6.3f\n",
	    Cell[i].S, sqrt(Cell[i].Beta[X_]*Cell[i].Beta[Y_]), 
            Cell[i].Beta[X_], Cell[i].Nu[X_], Cell[i].Beta[Y_], Cell[i].Nu[Y_],
	    Cell[i].Eta[X_], Cell[i].Eta[X_]*Cell[i].Beta[Y_],
            Cell[i].Nu[X_]-Cell[i].Nu[Y_]);
  }
  fclose(CodCorLatFile);
}

void prt_beamsizes()
{
  int   k;
  FILE  *fp;

  fp = file_write(beam_envelope_file);

  fprintf(fp,"# k  s  name s_xx s_pxpx s_xpx s_yy s_pypy s_ypy theta_xy\n");
  for(k = 0; k <= globval.Cell_nLoc; k++){
    fprintf(fp,"%4d %10s %e %e %e %e %e %e %e %e\n",
	    k, Cell[k].Elem.PName, Cell[k].S,
	    Cell[k].sigma[x_][x_], Cell[k].sigma[px_][px_],
	    Cell[k].sigma[x_][px_],
	    Cell[k].sigma[y_][y_], Cell[k].sigma[py_][py_],
	    Cell[k].sigma[y_][py_],
	    atan2(2e0*Cell[k].sigma[x_][y_],
		  Cell[k].sigma[x_][x_]-Cell[k].sigma[y_][y_])/2e0*180.0/M_PI);
  }

  fclose(fp);
}


float f_int_Touschek(const float u)
{
  double  f;

  if (u > 0.0)
    f = (1.0/u-log(1.0/u)/2.0-1.0)*exp(-u_Touschek/u); 
  else
    f = 0.0;

  return f;
} 

/****************************************************************************/
/* double Touschek(const double Qb, const double delta_RF,
		const double eps_x, const double eps_y,
		const double sigma_delta, const double sigma_s)

   Purpose:
       Calculate Touschek lifetime 
       using the normal theoretical formula 
       the momentum acceptance is RF acceptance delta_RF
      
   Parameters:   
             Qb  electric charge per bunch
       delta_RF  RF momentum acceptance
          eps_x  emittance x
	  eps_y  emittance y
    sigma_delta  longitudinal beam size
        sigma_s	   
   
   Input:
       none
               
   Output:
       

   Return:
      Touschek lifetime [second]

   Global variables:
       

   Specific functions:
      

   Comments:
       none

****************************************************************************/
double Touschek(const double Qb, const double delta_RF,
		const double eps_x, const double eps_y,
		const double sigma_delta, const double sigma_s)
{
  long int  k;
  double    tau, sigma_x, sigma_y, sigma_xp, L, curly_H;

  const double  gamma = 1e9*globval.Energy/m_e, N_e = Qb/q_e;

  printf("\n");
  printf("Qb = %4.2f nC, delta_RF = %4.2f%%"
	 ", eps_x = %9.3e nm.rad, eps_y = %9.3e nm.rad\n",
	 1e9*Qb, 1e2*delta_RF, 1e9*eps_x, 1e9*eps_y);
  printf("sigma_delta = %8.2e, sigma_s = %4.2f mm\n",
	 sigma_delta, 1e3*sigma_s);

  tau = 0.0;
  for(k = 1; k <= globval.Cell_nLoc; k++) {
    L = Cell[k].S - Cell[k-1].S;

    curly_H = get_curly_H(Cell[k].Alpha[X_], Cell[k].Beta[X_],
			  Cell[k].Eta[X_], Cell[k].Etap[X_]);

    // compute estimated beam sizes for given
    // hor.,  ver. emittance, sigma_s, and sigma_delta (x ~ 0)
    sigma_x = sqrt(Cell[k].Beta[X_]*eps_x+sqr(sigma_delta*Cell[k].Eta[X_])); 
    sigma_y = sqrt(Cell[k].Beta[Y_]*eps_y); 
    sigma_xp = (eps_x/sigma_x)*sqrt(1.0+curly_H*sqr(sigma_delta)/eps_x);  

    u_Touschek = sqr(delta_RF/(gamma*sigma_xp));

    tau += qromb(f_int_Touschek, 0.0, 1.0)/(sigma_x*sigma_y*sigma_xp)*L; 
    fflush(stdout);
  }

  tau *= N_e*sqr(r_e)*c0/(8.0*M_PI*cube(gamma)*sigma_s)
         /(sqr(delta_RF)*Cell[globval.Cell_nLoc].S);

  printf("\n"); 
  printf("Touschek lifetime [hrs]: %10.3e\n", 1.0/(3600.0*tau)); 
  return(1.0/(tau));
}

/****************************************************************************/
/* void mom_aper(double &delta, double delta_RF, const long int k,
	      const int n_turn, const bool positive)


   Purpose:
      Using Binary search to determine momentum aperture at location k.
      If the particle is stable at the last step of tracking, so the 
      momentum acceptance is RF momentum accpetance.
      
   Input:
           delta       particle energy deviation
	   delta_RF    RF momentum acceptance
	   k           element index
	   n_turn      number of tracking turns  

   Output:
           none
   Return:
           delta       searched momentum acceptance

   Global variables:
         none

   specific functions:
       Cell_Pass

   Comments:
       nsls-ii version. 
       See also:
       soleil version MomentumAcceptance( ) in soleillib.cc
       
****************************************************************************/
void mom_aper(double &delta, double delta_RF, const long int k,
	      const int n_turn, const bool positive)
{
  // Binary search to determine momentum aperture at location k.
  int       j;
  long int  lastpos;
  double    delta_min, delta_max;
  Vector    x;

  const double  eps = 1e-4;

  delta_min = 0.0; 
  delta_max = positive ? fabs(delta_RF) : -fabs(delta_RF);
  
  while (fabs(delta_max-delta_min) > eps) 
  {
    delta = (delta_max+delta_min)/2.0; /* binary serach*/
    
    // propagate initial conditions
    CopyVec(6, globval.CODvect, x); Cell_Pass(0, k, x, lastpos); 
    // generate Touschek event
    x[delta_] += delta;
    // complete one turn
    Cell_Pass(k, globval.Cell_nLoc, x, lastpos); 
    
    if (lastpos < globval.Cell_nLoc)
      // particle lost
      delta_max = delta;
    else { 
      // track 
      for(j = 0; j < n_turn; j++) {
	Cell_Pass(0, globval.Cell_nLoc, x, lastpos); 

	if ((delta_max > delta_RF) || (lastpos < globval.Cell_nLoc)) { 
	  // particle lost
	  delta_max = delta; 
	  break; 
	}
      }
      
      if ((delta_max <= delta_RF) && (lastpos == globval.Cell_nLoc))
	// particle not lost
	delta_min = delta; 
    }
  } 
}

/****************************************************************************/
/* double Touschek(const double Qb, const double delta_RF,const bool consistent,
		const double eps_x, const double eps_y,
		const double sigma_delta, double sigma_s,
		const int n_turn, const bool aper_on,
		double sum_delta[][2], double sum2_delta[][2])

   Purpose:
        Calculate the Touschek lifetime
	using the normal theoretical formula 
	the momentum acceptance is determined by the RF acceptance delta_RF 
	and the momentum aperture at each element location 
	which is tracked over n turns, 
        the vacuum chamber limitation is read from a outside file  
      
   Parameters:   
             Qb  electric charge per bunch
       delta_RF  RF momentum acceptance
     consistent
          eps_x  emittance x
	  eps_y  emittance y
    sigma_delta  longitudinal beam size
        sigma_s	
	 n_turn  number of turns for tracking  
	aper_on   must be TRUE
	         to indicate vacuum chamber setting is read from a outside file	        
 sum_delta[][2]	
sum2_delta[][2]		 
		  
   
   Input:
       none
               
   Output:
       

   Return:
       Touschek lifetime [second]

   Global variables:
       

   Specific functions:
      mom_aper

   Comments:
       none

****************************************************************************/
double Touschek(const double Qb, const double delta_RF,const bool consistent,
		const double eps_x, const double eps_y,
		const double sigma_delta, double sigma_s,
		const int n_turn, const bool aper_on,
		double sum_delta[][2], double sum2_delta[][2])
{
  bool      cav, aper;
  long int  k;
  double    rate, delta_p, delta_m, curly_H0, curly_H1, L;
  double    sigma_x, sigma_y, sigma_xp;

  const bool  prt = false;

  //  const char  file_name[] = "Touschek.out";
  const double  eps = 1e-5, gamma = 1e9*globval.Energy/m_e, N_e = Qb/q_e;

  cav = globval.Cavity_on; aper = globval.Aperture_on;

  globval.Cavity_on = true;

  Ring_GetTwiss(true, 0.0);

  globval.Aperture_on = aper_on;

  printf("\n");
  printf("Qb = %4.2f nC, delta_RF = %4.2f%%"
	 ", eps_x = %9.3e m.rad, eps_y = %9.3e m.rad\n",
	 1e9*Qb, 1e2*delta_RF, eps_x, eps_y);
  printf("sigma_delta = %8.2e, sigma_s = %4.2f mm\n",
	 sigma_delta, 1e3*sigma_s);

  printf("\n");
  printf("Momentum aperture:" );
  printf("\n");

  delta_p = delta_RF; mom_aper(delta_p, delta_RF, 0, n_turn, true);
  delta_m = -delta_RF; mom_aper(delta_m, delta_RF, 0, n_turn, false);
  delta_p = min(delta_RF, delta_p); delta_m = max(-delta_RF, delta_m);
  sum_delta[0][0] += delta_p; sum_delta[0][1] += delta_m;
  sum2_delta[0][0] += sqr(delta_p); sum2_delta[0][1] += sqr(delta_m);
  
  rate = 0.0; curly_H0 = -1e30;
  for (k = 1; k <= globval.Cell_nLoc; k++) {
    L = Cell[k].S - Cell[k-1].S;

    curly_H1 = get_curly_H(Cell[k].Alpha[X_], Cell[k].Beta[X_],
			   Cell[k].Eta[X_], Cell[k].Etap[X_]);

    if (fabs(curly_H0-curly_H1) > eps) {
      mom_aper(delta_p, delta_RF, k, n_turn, true);
      delta_m = -delta_p; mom_aper(delta_m, delta_RF, k, n_turn, false);
      delta_p = min(delta_RF, delta_p); delta_m = max(-delta_RF, delta_m);
      printf("%4ld %6.2f %3.2lf%% %3.2lf%%\n",
	     k, Cell[k].S, 1e2*delta_p, 1e2*delta_m);
      curly_H0 = curly_H1;
    }

    sum_delta[k][0] += delta_p; 
    sum_delta[k][1] += delta_m;
    sum2_delta[k][0] += sqr(delta_p); 
    sum2_delta[k][1] += sqr(delta_m);
    
    if (prt)
      printf("%4ld %6.2f %3.2lf %3.2lf\n",
	     k, Cell[k].S, 1e2*delta_p, 1e2*delta_m);

    if (!consistent) {
      // compute estimated beam sizes for given
      // hor.,  ver. emittance, sigma_s, and sigma_delta (x ~ 0)
      sigma_x = sqrt(Cell[k].Beta[X_]*eps_x+sqr(sigma_delta*Cell[k].Eta[X_]));
      sigma_y = sqrt(Cell[k].Beta[Y_]*eps_y);
      sigma_xp = (eps_x/sigma_x)*sqrt(1.0+curly_H1*sqr(sigma_delta)/eps_x);
    } else {
      // use self-consistent beam sizes
      sigma_x = sqrt(Cell[k].sigma[x_][x_]);
      sigma_y = sqrt(Cell[k].sigma[y_][y_]);
      sigma_xp = sqrt(Cell[k].sigma[px_][px_]);
      sigma_s = sqrt(Cell[k].sigma[ct_][ct_]);
    }

    u_Touschek = sqr(delta_p/(gamma*sigma_xp));

    rate += qromb(f_int_Touschek, 0.0, 1.0)
            /(sigma_x*sigma_xp*sigma_y*sqr(delta_p))*L;

    fflush(stdout);
  }

  rate *= N_e*sqr(r_e)*c0/(8.0*M_PI*cube(gamma)*sigma_s)
         /Cell[globval.Cell_nLoc].S;

  printf("\n");
  printf("Touschek lifetime [hrs]: %4.2f\n", 1.0/(3600.0*rate));

  globval.Cavity_on = cav; globval.Aperture_on = aper;

  return 1/rate;
}


double f_IBS(const double chi_m)
{
  // Interpolated

  double  f, ln_chi_m;

  const double A = 1.0, B = 0.579, C = 0.5;

  ln_chi_m = log(chi_m); f = A + B*ln_chi_m + C*sqr(ln_chi_m);

  return f;
}


float f_int_IBS(const float chi)
{
  // Integrated

  double  f;

  f = exp(-chi)*log(chi/chi_m)/chi;

  return f;
}


void IBS(const double Qb,
	 const double eps_SR[], double eps[],
	 const double alpha_z, const double beta_z)
{
  /* The equilibrium emittance is given by:

       sigma_delta^2 = tau_delta*(D_delta_IBS + D_delta_SR)

       D_x = <D_delta*curly_H> =>

       eps_x = eps_x_SR + eps_x_IBS,    D_x_IBS ~ 1/eps_x

     i.e., the 2nd term is essentially constant.  From

       eps_x^2 = eps_x*eps_x_SR + eps_x*eps_x_IBS

     one obtains the IBS limited emittance (eps_x_SR = 0, eps_x^2=eps_x_IBS*tau_x*D_x_IBS->cst):

       eps_x,IBS = sqrt(tau_x*(eps_x*D_x_IBS)

     and by solving for eps_x

       eps_x = eps_x_SR/2 + sqrt(eps_x_SR^2/4+eps_x_IBS^2)

  */

  long int  k;
  double    D_x, D_delta, b_max, L, gamma_z;
  double    sigma_x, sigma_xp, sigma_y, sigma_p, sigma_s, sigma_delta;
  double    incr, C, curly_H, eps_IBS[3], eps_x_tot, sigma_E;
  double    sigma_s_SR, sigma_delta_SR, sigma_delta_IBS;

  const bool    integrate = false;
  const double  gamma = 1e9*globval.Energy/m_e, N_e = Qb/q_e;

  // bunch size
  gamma_z = (1.0+sqr(alpha_z))/beta_z;
  sigma_s_SR = sqrt(beta_z*eps_SR[Z_]);
  sigma_delta_SR = sqrt(gamma_z*eps_SR[Z_]);

  sigma_s = sqrt(beta_z*eps[Z_]); sigma_delta = sqrt(gamma_z*eps[Z_]);

  printf("\n");
  printf("Qb             = %4.2f nC\n", 1e9*Qb);
  printf("eps_x_SR       = %9.3e m.rad, eps_x       = %9.3e m.rad\n",
	 eps_SR[X_], eps[X_]);
  printf("eps_y_SR       = %9.3e m.rad, eps_y       = %9.3e m.rad\n",
	 eps_SR[Y_], eps[Y_]);
  printf("eps_z_SR       = %9.3e,       eps_z       = %9.3e\n",
	 eps_SR[Z_], eps[Z_]);
  printf("alpha_z        = %9.3e,       beta_z      = %9.3e\n",
	 alpha_z, beta_z);
  printf("sigma_s_SR     = %9.3e mm,    sigma_s     = %9.3e mm\n",
	 1e3*sigma_s_SR, 1e3*sigma_s);
  printf("sigma_delta_SR = %9.3e,       sigma_delta = %9.3e\n",
	 sigma_delta_SR, sigma_delta);

  D_delta = 0.0; D_x = 0.0;
  for(k = 1; k <= globval.Cell_nLoc; k++) {
    L = Cell[k].Elem.PL;

    curly_H = get_curly_H(Cell[k].Alpha[X_], Cell[k].Beta[X_],
			  Cell[k].Eta[X_], Cell[k].Etap[X_]);

    // compute estimated beam sizes for given
    // hor.,  ver. emittance, sigma_s, and sigma_delta (x ~ 0)
    sigma_x = sqrt(Cell[k].Beta[X_]*eps[X_]+sqr(sigma_delta*Cell[k].Eta[X_])); 
    sigma_y = sqrt(Cell[k].Beta[Y_]*eps[Y_]);
    sigma_xp = (eps[X_]/sigma_x)*sqrt(1.0+curly_H*sqr(sigma_delta)/eps[X_]);

    sigma_E = gamma*m_e*q_e*sigma_delta;
    sigma_p = gamma*m_e*q_e*sigma_xp;

//    b_max = sqrt(2.0*M_PI)/pow(N_e/(sigma_x*sigma_y*sigma_s), 1.0/3.0);
    b_max = 2.0*sqrt(M_PI)/pow(N_e/(sigma_x*sigma_y*sigma_s), 1.0/3.0);

    chi_m = r_e*sqr(m_e*q_e/sigma_E)/b_max;
//    chi_m = r_e*sqr(m_e*q_e/sigma_p)/b_max;

    if (!integrate)
      incr = f_IBS(chi_m)/sigma_y*L;
    else
      incr = qromb(f_int_IBS, chi_m, 1e5*chi_m)/sigma_y*L;

    D_delta += incr; D_x += incr*curly_H;
  }

  C = Cell[globval.Cell_nLoc].S;

  if (true) {
    eps_x_tot = 1.0;
    D_delta *= N_e*sqr(r_e)*c0/(32.0*M_PI*cube(gamma)*eps_x_tot*sigma_s*C);
    D_x *= N_e*sqr(r_e)*c0/(32.0*M_PI*cube(gamma)*eps_x_tot*sigma_s*C);
    eps_IBS[X_] = sqrt(globval.tau[X_]*D_x);
    eps_x_tot = eps_SR[X_]*(1.0+sqrt(1.0+4.0*sqr(eps_IBS[X_]/eps_SR[X_])))/2.0;
    D_delta /= eps_x_tot; D_x /= eps_x_tot;
    sigma_delta_IBS = sqrt(globval.tau[Z_]*D_delta);
    eps[X_] = eps_x_tot;

    eps[Y_] = eps_SR[Y_]/eps_SR[X_]*eps[X_];
    sigma_delta = sqrt(sqr(sigma_delta_SR)+sqr(sigma_delta_IBS));
    eps_IBS[Z_] = sigma_s*sigma_delta;
    eps[Z_] = eps_SR[Z_] + eps_IBS[Z_];
  } else {
    D_delta *= N_e*sqr(r_e)*c0/(32.0*M_PI*cube(gamma)*eps[X_]*sigma_s*C);
    D_x *= N_e*sqr(r_e)*c0/(32.0*M_PI*cube(gamma)*eps[X_]*sigma_s*C);
    eps_IBS[X_] = globval.tau[X_]*D_x;
    eps_IBS[Z_] = globval.tau[Z_]*D_delta;
    eps[X_] = eps_SR[X_] + eps_IBS[X_];
    eps[Y_] = eps_SR[Y_]/eps_SR[X_]*eps[X_];
    sigma_delta_IBS = sqrt(globval.tau[Z_]*D_delta);
    sigma_delta = sqrt(sqr(sigma_delta_SR)+sqr(sigma_delta_IBS));
    // approx.
    eps_IBS[Z_] = sigma_s*sigma_delta;
    eps[Z_] = eps_SR[Z_] + eps_IBS[Z_];
  }

  // bunch size
  sigma_s = sqrt(beta_z*eps[Z_]);
  sigma_delta = sqrt(gamma_z*eps[Z_]);

  printf("\n");
  printf("D_x,IBS         = %9.3e\n", D_x);
  printf("eps_x,IBS       = %5.3f nm.rad\n", 1e9*eps_IBS[X_]);
  printf("eps_x,tot       = %5.3f nm.rad\n", 1e9*eps[X_]);
  printf("eps_y,tot       = %5.3f nm.rad\n", 1e9*eps[Y_]);
  printf("eps_z,tot       = %9.3e\n", eps[Z_]);
  printf("D_delta,IBS     = %9.3e\n", D_delta);
  printf("sigma_delta,IBS = %9.3e\n", sigma_delta_IBS);
  printf("sigma_delta,tot = %9.3e\n", sigma_delta);
  printf("sigma_s,tot     = %9.3e\n", sigma_s);
}


void rm_space(char *name)
{
  int  i, k;

  i = 0;
  while (name[i] == ' ')
    i++;

  for (k = i; k <= (int)strlen(name)+1; k++)
    name[k-i] = name[k];
}


void get_bn(char file_name[], int n, const bool prt)
{
  char      line[max_str], str[max_str], str1[max_str], *token, *name;
  int       n_prm, Fnum, Knum, order;
  double    bnL, bn, C, L;
  FILE      *inf, *fp_lat;

  inf = file_read(file_name); fp_lat = file_write("get_bn.lat");

  // if n = 0: go to last data set
  if (n == 0) {
    while (fgets(line, max_str, inf) != NULL )
      if (strstr(line, "n = ") != NULL)	sscanf(line, "n = %d", &n);

    fclose(inf); inf = file_read(file_name);
  }

  if (prt) {
    printf("\n");
    printf("reading values (n=%d): %s\n", n, file_name);
    printf("\n");
  }

  sprintf(str, "n = %d", n);
  do
    fgets(line, max_str, inf);
  while (strstr(line, str) == NULL);

  fprintf(fp_lat, "\n");
  n_prm = 0;
  while (fgets(line, max_str, inf) != NULL) {
    if (strcmp(line, "\n") == 0) break;
    n_prm++;
    name = strtok(line, "(");
    rm_space(name);
    strcpy(str, name); Fnum = ElemIndex(str);
    strcpy(str1, name); upr_case(str1);
    token = strtok(NULL, ")"); sscanf(token, "%d", &Knum);
    strtok(NULL, "="); token = strtok(NULL, "\n");
    sscanf(token, "%lf %d", &bnL, &order);
    if (prt) printf("%6s(%2d) = %10.6f %d\n", name, Knum, bnL, order);
   if (Fnum != 0) {
      if (order == 0)
        SetL(Fnum, bnL);
      else
        SetbnL(Fnum, order, bnL);
    
      L = GetL(Fnum, 1);
      if (Knum == 1)
	if (order == 0)
	  fprintf(fp_lat, "%s: Drift, L = %8.6f;\n", str1, bnL);
	else {
	  bn = (L != 0.0)? bnL/L : bnL;
	  if (order == Quad)
	    fprintf(fp_lat, "%s: Quadrupole, L = %8.6f, K = %10.6f, N = Nquad"
		    ", Method = Meth;\n", str1, L, bn);
	  else if (order == Sext)
	    fprintf(fp_lat, "%s: Sextupole, L = %8.6f, K = %10.6f"
		    ", N = 1, Method = Meth;\n", str1, L, bn);
	  else {
	    fprintf(fp_lat, "%s: Multipole, L = %8.6f"
		    ", N = 1, Method = Meth,\n", str1, L);
	    fprintf(fp_lat, "     HOM = (%d, %13.6f, %3.1f);\n",
		    order, bn, 0.0);
	  }
	}
    } else {
      printf("element %s not found\n", name);
      exit_(1);
    }
  }
  if (prt) printf("\n");

  C = Cell[globval.Cell_nLoc].S; recalc_S();
  if (prt)
    printf("New Cell Length: %5.3f (%5.3f)\n", Cell[globval.Cell_nLoc].S, C);

  fclose(inf); fclose(fp_lat);
}


double get_dynap(const double delta)
{
  char      str[max_str];
  int       i;
  double    x_aper[n_aper], y_aper[n_aper], DA;
  FILE      *fp;

  const int  prt = true;

  fp = file_write("dynap.out");
  dynap(fp, 5e-3, 0.0, 0.1e-3, n_aper, n_track, x_aper, y_aper, false, prt);
  fclose(fp);
  DA = get_aper(n_aper, x_aper, y_aper);

  if (true) {
    sprintf(str, "dynap_dp%3.1f.out", 1e2*delta);
    fp = file_write(str);
    dynap(fp, 5e-3, delta, 0.1e-3, n_aper, n_track,
	  x_aper, y_aper, false, prt);
    fclose(fp);
    DA += get_aper(n_aper, x_aper, y_aper);

    for (i = 0; i < nv_; i++)
      globval.CODvect[i] = 0.0;
    sprintf(str, "dynap_dp%3.1f.out", -1e2*delta);
    fp = file_write(str);
    dynap(fp, 5e-3, -delta, 0.1e-3, n_aper,
	  n_track, x_aper, y_aper, false, prt);
    fclose(fp);
    DA += get_aper(n_aper, x_aper, y_aper);
  }

  return DA/3.0;
}


double get_chi2(long int n, double x[], double y[], long int m, Vector b)
{
  /* Compute chi2 for polynomial fit */

  int     i, j;
  double  sum, z;
  
  sum = 0.0;
  for (i = 0; i < n; i++) {
    z = 0.0;
    for (j = 0; j <= m; j++)
      z += b[j]*pow(x[i], j);
    sum += pow(y[i]-z, 2);
  }
  return(sum/n);
}


void pol_fit(int n, double x[], double y[], int order, Vector &b,
	     double &sigma)
{
  /* Polynomial fit by linear chi-square */

  int     i, j, k;
  Matrix  T1;
  bool   prt = false;
  
  const	double sigma_k = 1.0, chi2 = 4.0;

  /* initialization */
  for (i = 0; i <= order; i++) {
    b[i] = 0.0;
    for (j = 0; j <= order; j++)
      T1[i][j] = 0.0;
  }

  /* polynomal fiting */
  for (i = 0; i < n; i++)
    for (j = 0; j <= order; j++) {
      b[j] += y[i]*pow(x[i], j)/pow(sigma_k, 2);
      for (k = 0; k <= order; k++)
	T1[j][k] += pow(x[i], j+k)/pow(sigma_k, 2);
    }

  if (InvMat(order+1, T1)) {
    LinTrans(order+1, T1, b); sigma = get_chi2(n, x, y, order, b);
    if(prt){
      printf("\n");
      printf("  n    Coeff.\n");
      for (i = 0; i <= order; i++)
        printf("%3d %10.3e+/-%8.2e\n", i, b[i], sigma*sqrt(chi2*T1[i][i]));
    }	
  } else {
    printf("pol_fit: Matrix is singular\n");
    exit_(1);
  }
}


void get_ksi2(const double d_delta)
{
  const int     n_points = 20, order = 5;

  int       i, n;
  double    delta[2*n_points+1], nu[2][2*n_points+1], sigma;
  Vector    b;
  FILE      *fp;
  
  fp = file_write("chrom2.out");
  n = 0;
  for (i = -n_points; i <= n_points; i++) {
    n++; delta[n-1] = i*(double)d_delta/(double)n_points;
    Ring_GetTwiss(false, delta[n-1]);
    nu[0][n-1] = globval.TotalTune[X_]; nu[1][n-1] = globval.TotalTune[Y_];
    fprintf(fp, "%5.2f %8.5f %8.5f\n", 1e2*delta[n-1], nu[0][n-1], nu[1][n-1]);
  }
  printf("\n");
  printf("Horizontal chromaticity:\n");
  pol_fit(n, delta, nu[0], order, b, sigma);
  printf("\n");
  printf("Vertical chromaticity:\n");
  pol_fit(n, delta, nu[1], order, b, sigma);
  printf("\n");
  fclose(fp);
}


bool find_nu(const int n, const double nus[], const double eps, double &nu)
{
  bool  lost;
  int   k;

  const bool  prt = false;

  if (prt) printf("\n");
  k = 0;
  while ((k < n) && (fabs(nus[k]-nu) > eps)) {
    if (prt) printf("nu = %7.5f(%7.5f) eps = %7.1e\n", nus[k], nu, eps);
    k++;
  }

  if (k < n) {
    if (prt) printf("nu = %7.5f(%7.5f)\n", nus[k], nu);
    nu = nus[k]; lost = false;
  } else {
    if (prt) printf("lost\n");
    lost = true;
  }

  return !lost;
}


bool get_nu(const double Ax, const double Ay, const double delta,
	    const double eps, double &nu_x, double &nu_y)
{
  const int  n_turn = 512, n_peaks = 5;

  bool      lost, ok_x, ok_y;
  char      str[max_str];
  int       i;
  long int  lastpos, lastn, n;
  double    x[n_turn], px[n_turn], y[n_turn], py[n_turn];
  double    nu[2][n_peaks], A[2][n_peaks];
  Vector    x0;
  FILE     *fp;

  const bool   prt = false;
  const char   file_name[] = "track.out";

  // complex FFT in Floquet space
  x0[x_] = Ax; x0[px_] = 0.0; x0[y_] = Ay; x0[py_] = 0.0;
  LinTrans(4, globval.Ascrinv, x0);
  track(file_name, x0[x_], x0[px_], x0[y_], x0[py_], delta,
	n_turn, lastn, lastpos, 1, 0.0);
  if (lastn == n_turn) {
    GetTrack(file_name, &n, x, px, y, py);
    sin_FFT((int)n, x, px); sin_FFT((int)n, y, py);

    if (prt) {
      strcpy(str, file_name); strcat(str, ".fft");
      fp = file_write(str);
      for (i = 0; i < n; i++)
	fprintf(fp, "%5.3f %12.5e %8.5f %12.5e %8.5f\n",
		(double)i/(double)n, x[i], px[i], y[i], py[i]);
      fclose(fp);
    }
    
    GetPeaks1(n, x, n_peaks, nu[X_], A[X_]);
    GetPeaks1(n, y, n_peaks, nu[Y_], A[Y_]);

    ok_x = find_nu(n_peaks, nu[X_], eps, nu_x);
    ok_y = find_nu(n_peaks, nu[Y_], eps, nu_y);

    lost = !ok_x || !ok_y;
  } else
    lost = true;
  
  return !lost;
}


// tune shift with amplitude
void dnu_dA(const double Ax_max, const double Ay_max, const double delta)
{
  bool      ok;
  int       i;
  double    nu_x, nu_y, Ax, Ay, Jx, Jy;
  Vector    ps;
  FILE      *fp;

  const int     n_ampl = 25;
  const double  A_min  = 1e-3;
//  const double  eps0   = 0.04, eps   = 0.02;
//  const double  eps0   = 0.025, eps   = 0.02;
  const double  eps0   = 0.04, eps   = 0.015;

  Ring_GetTwiss(false, 0.0);

  ok = false;
  fp = file_write("dnu_dAx.out");
  fprintf(fp, "#   x[mm]        y[mm]        J_x        J_y      nu_x    nu_y\n");
  fprintf(fp, "#\n");
  for (i = -n_ampl; i <= n_ampl; i++) {
    Ax = i*Ax_max/n_ampl;
    if (Ax == 0.0) Ax = A_min;
    Ay = A_min;
    ps[x_] = Ax; ps[px_] = 0.0; ps[y_] = Ay; ps[py_] = 0.0; getfloqs(ps);
    Jx = (sqr(ps[x_])+sqr(ps[px_]))/2.0; Jy = (sqr(ps[y_])+sqr(ps[py_]))/2.0;
    if (!ok) {
      nu_x = fract(globval.TotalTune[X_]); nu_y = fract(globval.TotalTune[Y_]);
      ok = get_nu(Ax, Ay, delta, eps0, nu_x, nu_y);
    } else
      ok = get_nu(Ax, Ay, delta, eps, nu_x, nu_y);
    if (ok)
      fprintf(fp, "%10.3e %10.3e %10.3e %10.3e %7.5f %7.5f\n",
	      1e3*Ax, 1e3*Ay, 1e6*Jx, 1e6*Jy, fract(nu_x), fract(nu_y));
    else
      fprintf(fp, "# %10.3e %10.3e particle lost\n", 1e3*Ax, 1e3*Ay);
  }
  fclose(fp);

  ok = false;
  fp = file_write("dnu_dAy.out");
  fprintf(fp, "#   x[mm]        y[mm]      J_x        J_y      nu_x    nu_y\n");
  fprintf(fp, "#\n");
  for (i = -n_ampl; i <= n_ampl; i++) {
    Ax = A_min; Ay = i*Ay_max/n_ampl;
    Jx = pow(Ax, 2)/(2.0*Cell[globval.Cell_nLoc].Beta[X_]);
    if (Ay == 0.0) Ay = A_min;
    Jy = pow(Ay, 2)/(2.0*Cell[globval.Cell_nLoc].Beta[Y_]);
//    if (i == 1) exit_(0);
    if (!ok) {
      nu_x = fract(globval.TotalTune[X_]); nu_y = fract(globval.TotalTune[Y_]);
      ok = get_nu(Ax, Ay, delta, eps0, nu_x, nu_y);
    } else
      ok = get_nu(Ax, Ay, delta, eps, nu_x, nu_y);
    if (ok)
      fprintf(fp, "%10.3e %10.3e %10.3e %10.3e %7.5f %7.5f\n",
	      1e3*Ax, 1e3*Ay, 1e6*Jx, 1e6*Jy, fract(nu_x), fract(nu_y));
    else
      fprintf(fp, "# %10.3e %10.3e particle lost\n", 1e3*Ax, 1e3*Ay);
  }
  fclose(fp);
}


bool orb_corr(const int n_orbit)
{
  bool      cod = false;
  int       i;
  long      lastpos;
  Vector2   xmean, xsigma, xmax;

  printf("\n");

//  FitTune(qf, qd, nu_x, nu_y);
//  printf("\n");
//  printf("  qf = %8.5f qd = %8.5f\n",
//	   GetKpar(qf, 1, Quad), GetKpar(qd, 1, Quad));

  globval.CODvect.zero();
  for (i = 1; i <= n_orbit; i++) {
    cod = getcod(0.0, lastpos);
    if (cod) {
      codstat(xmean, xsigma, xmax, globval.Cell_nLoc, false);
      printf("\n");
      printf("RMS orbit [mm]: (%8.1e+/-%7.1e, %8.1e+/-%7.1e)\n", 
	     1e3*xmean[X_], 1e3*xsigma[X_], 1e3*xmean[Y_], 1e3*xsigma[Y_]);
      lsoc(1, globval.bpm, globval.hcorr, 1);
      lsoc(1, globval.bpm, globval.vcorr, 2);
      cod = getcod(0.0, lastpos);
      if (cod) {
	codstat(xmean, xsigma, xmax, globval.Cell_nLoc, false);
	printf("RMS orbit [mm]: (%8.1e+/-%7.1e, %8.1e+/-%7.1e)\n", 
	       1e3*xmean[X_], 1e3*xsigma[X_], 1e3*xmean[Y_], 1e3*xsigma[Y_]);
      } else
	printf("orb_corr: failed\n");
    } else
      printf("orb_corr: failed\n");
  }
  
  prt_cod("orb_corr.out", globval.bpm, true);

  return cod;
}


double get_code(CellType &Cell)
{
  double  code = 0.0;

  switch (Cell.Elem.Pkind) {
  case drift:
    code = 0.0;
    break;
  case Mpole:
    if (Cell.Elem.M->Pirho != 0.0) {
      code = 0.5;
    } else if (Cell.Elem.M->PBpar[Quad+HOMmax] != 0)
      code = sgn(Cell.Elem.M->PBpar[Quad+HOMmax]);
    else if (Cell.Elem.M->PBpar[Sext+HOMmax] != 0)
      code = 1.5*sgn(Cell.Elem.M->PBpar[Sext+HOMmax]);
    else if (Cell.Fnum == globval.bpm)
      code = 2.0;
    else
      code = 0.0;
    break;
  default:
    code = 0.0;
    break;
  }

  return code;
}


void get_alphac(void)
{
  CellType  Cell;

  getelem(globval.Cell_nLoc, &Cell);
  globval.Alphac = globval.OneTurnMat[ct_][delta_]/Cell.S;
}

/******************************************************************
void get_alphac2(void)

  Purpose:
           Calculate momentum compact factor up to third order.
	   Method:
	   track the orbit offset c*tau at the first element,
	   at 11 different energy offset(-10-3 to 10-3), then 
	   use polynomal to fit the momentum compact faction factor
	   up to 3rd order.
	   The initial coorinates are (x_co,px_co,y_co,py_co,delta,0).
	   
 Input:
       none

   Output:
       none

   Return:
      alphac

   specific functions:
      none

   Comments:
       change the initial coorinates from (0 0 0 0 delta 0) to 
              (x_co,px_co,y_co,py_co,delta,0).	   

*******************************************************************/

void get_alphac2(void)
{
  /* Note, do not extract from M[5][4], i.e. around delta
     dependent fixed point.                                */

  const int     n_points = 5;
  const double  d_delta  = 1e-3;
 //const double  d_delta  = 1e-4;

  int       i, n;
  long int  lastpos;  // last tracking position when the particle is stable 
  double    delta[2*n_points+1], alphac[2*n_points+1], sigma;
  Vector    x, b;
  CellType  Cell;
  bool      cod;
  
  globval.pathlength = false;
  getelem(globval.Cell_nLoc, &Cell); 
  n = 0;
  
  for (i = -n_points; i <= n_points; i++) {
    n++; 
    delta[n-1] = i*(double)d_delta/(double)n_points;
    
   // for (j = 0; j < nv_; j++)
    //  x[j] = 0.0;
    //   x[delta_] = delta[n-1];
    
    cod = getcod(delta[n-1], lastpos);   
    x =  globval.CODvect;
        
   
      Cell_Pass(0, globval.Cell_nLoc, x, lastpos);
      alphac[n-1] = x[ct_]/Cell.S; // this is not mcf but DT/T
  }
  pol_fit(n, delta, alphac, 3, b, sigma);
  printf("Momentum compaction factor:\n");
  printf("   alpha1 = %10.4e  alpha2 = %10.4e  alpha3 = %10.4e\n",
	 b[1], b[2], b[3]);
}


float f_bend(float b0L[])
{
  long int  lastpos;
  Vector    ps;

  const int   n_prt = 10;

  n_iter_Cart++;

  SetbnL_sys(Fnum_Cart, Dip, b0L[1]);

  ps.zero();
  Cell_Pass(Elem_GetPos(Fnum_Cart, 1)-1, Elem_GetPos(Fnum_Cart, 1),
	    ps, lastpos);

  if (n_iter_Cart % n_prt == 0)
    cout << scientific << setprecision(3)
	 << setw(4) << n_iter_Cart
	 << setw(11) << ps[x_] << setw(11) << ps[px_] << setw(11) << ps[ct_]
	 << setw(11) << b0L[1] << endl;

  return sqr(ps[x_]) + sqr(ps[px_]);
}


void bend_cal_Fam(const int Fnum)
{
  /* Adjusts b1L_sys to zero the orbit for a given gradient. */
  const int  n_prm = 1;

  int       iter;
  float     *b0L, **xi, fret;
  Vector    ps;

  const float  ftol = 1e-15;

  b0L = vector(1, n_prm); xi = matrix(1, n_prm, 1, n_prm);

  cout << endl;
  cout << "bend_cal: " << ElemFam[Fnum-1].ElemF.PName << ":" << endl;

  Fnum_Cart = Fnum;  b0L[1] = 0.0; xi[1][1] = 1e-3;

  cout << endl;
  n_iter_Cart = 0;
  powell(b0L, xi, n_prm, ftol, &iter, &fret, f_bend);

  free_vector(b0L, 1, n_prm); free_matrix(xi, 1, n_prm, 1, n_prm);
}


void bend_cal(void)
{
  long int  k;

  for (k = 1; k <= globval.Elem_nFam; k++)
    if ((ElemFam[k-1].ElemF.Pkind == Mpole) &&
	(ElemFam[k-1].ElemF.M->Pirho != 0.0) &&
	(ElemFam[k-1].ElemF.M->PBpar[Quad+HOMmax] != 0.0))
      if (ElemFam[k-1].nKid > 0) bend_cal_Fam(k);
}


double h_ijklm(const tps &h, const int i, const int j, const int k,
	       const int l, const int m)
{
  int      i1;
  iVector  jj;

  for (i1 = 0; i1 < nv_tps; i1++)
    jj[i1] = 0;
  jj[x_] = i; jj[px_] = j; jj[y_] = k; jj[py_] = l; jj[delta_] = m;
  return h[jj];
}


ss_vect<tps> get_A(const double alpha[], const double beta[],
		   const double eta[], const double etap[])
{
  ss_vect<tps>  A, Id;

  Id.identity();

  A.identity();
  A[x_]  = sqrt(beta[X_])*Id[x_];
  A[px_] = -alpha[X_]/sqrt(beta[X_])*Id[x_] + 1.0/sqrt(beta[X_])*Id[px_];
  A[y_]  = sqrt(beta[Y_])*Id[y_];
  A[py_] = -alpha[Y_]/sqrt(beta[Y_])*Id[y_] + 1.0/sqrt(beta[Y_])*Id[py_];

  A[x_] += eta[X_]*Id[delta_]; A[px_] += etap[X_]*Id[delta_];

  return A;
}


void get_ab(const ss_vect<tps> &A,
	    double alpha[], double beta[], double eta[], double etap[])
{
  int           k;
  ss_vect<tps>  A_Atp;

  A_Atp = A*tp_S(2, A);

  for (k = 0; k <= 1; k++) {
      eta[k] = A[2*k][delta_]; 
     etap[k] = A[2*k+1][delta_];
    alpha[k] = -A_Atp[2*k][2*k+1]; 
     beta[k] = A_Atp[2*k][2*k];
  }
}


void set_tune(const char file_name1[], const char file_name2[], const int n)
{
  const int  n_b2 = 8;

  char      line[max_str], names[n_b2][max_str];
  int       j, k, Fnum;
  double    b2s[n_b2], nu[2];
  ifstream  inf1, inf2;
  FILE      *fp_lat;

  file_rd(inf1, file_name1); file_rd(inf2, file_name2);

  // skip 1st and 2nd line
  inf1.getline(line, max_str); inf2.getline(line, max_str);
  inf1.getline(line, max_str); inf2.getline(line, max_str);
  
  inf1.getline(line, max_str);
  sscanf(line, "%*s %*s %*s %s %s %s %s",
	 names[0], names[1], names[2], names[3]);
  inf2.getline(line, max_str);
  sscanf(line, "%*s %*s %*s %s %s %s %s",
	 names[4], names[5], names[6], names[7]);

  printf("\n");
  printf("quads:");
  for (k = 0; k <  n_b2; k++) {
    lwr_case(names[k]); rm_space(names[k]);
    printf(" %s", names[k]);
  }
  printf("\n");

  // skip 4th line
  inf1.getline(line, max_str); inf2.getline(line, max_str);

  fp_lat = file_write("set_tune.lat");
  while (inf1.getline(line, max_str) != NULL) {
    sscanf(line, "%d%lf %lf %lf %lf %lf %lf",
	   &j, &nu[X_], &nu[Y_], &b2s[0], &b2s[1], &b2s[2], &b2s[3]);
    inf2.getline(line, max_str);
    sscanf(line, "%d%lf %lf %lf %lf %lf %lf",
	   &j, &nu[X_], &nu[Y_], &b2s[4], &b2s[5], &b2s[6], &b2s[7]);

    if (j == n) {
      printf("\n");
      printf("%3d %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f %8.5f\n",
	     n,
	     b2s[0], b2s[1], b2s[2], b2s[3], b2s[4], b2s[5], b2s[6], b2s[7]);

      for (k = 0; k <  n_b2; k++) {
	Fnum = ElemIndex(names[k]);
	set_bn_design_fam(Fnum, Quad, b2s[k], 0.0);

	fprintf(fp_lat, "%s: Quadrupole, L = %8.6f, K = %10.6f, N = Nquad"
		", Method = Meth;\n",
		names[k], ElemFam[Fnum-1].ElemF.PL, b2s[k]);
      }
      break;
    }
  }

  fclose(fp_lat);
}





