/* Tracy-2

   J. Bengtsson, CBP, LBL      1990 - 1994   Pascal version
                 SLS, PSI      1995 - 1997
   M. Boege      SLS, PSI      1998          C translation
   L. Nadolski   SOLEIL        2002          Link to NAFF, Radia field maps
   J. Bengtsson  NSLS-II, BNL  2004 -        

   t2ring.c -- Routines for closed beam lines

*/


#include "tracy_lib.h"

#define n  4


void GetNu(Vector2 &nu, Matrix &M)
{
  int     i;
  double  sgn, detp, detm, b, c, th, tv, b2mc, x_arg, y_arg;
  Matrix  M1;

  globval.stable = true;

  CopyMat((long)n, M, M1);

  for (i = 0; i < n; i++)
    M1[i][i] -= 1.0;   
  detp = DetMat((long)n, M1); /* det(M-I) */

  for (i = 0; i < n; i++)
    M1[i][i] += 2.0;
  detm = DetMat((long)n, M1); /* det(M+I) */

  for (i = 0; i < n; i++)
    M1[i][i] -= 1.0; /* restore M */

  b = (detp-detm)/16.0; c = (detp+detm)/8.0 - 1.0;
  th = (M1[0][0]+M1[1][1])/2.0; tv = (M1[2][2]+M1[3][3])/2.0;

  b2mc = b*b - c;

  if (b2mc < 0.0) {
    globval.stable = false; nu[0] = -1.0; nu[1] = -1.0;
    printf("GetNu: unstable\n");
    return;
  }

  sgn = (th > tv)? 1.0 : -1.0;

  x_arg = -b + sgn*sqrt(b2mc);
  if (fabs(x_arg) <= 1.0) {
    nu[X_] = acos(x_arg)/(2.0*M_PI);
    if (M1[0][1] < 0.0) nu[X_] = 1.0 - nu[X_];
  } else {
    globval.stable = false; nu[X_] = -1.0;
    printf("GetNu: unstable (horizontal)\n");
  }

  y_arg = -b - sgn*sqrt(b2mc);
  if (fabs(y_arg) <= 1.0) {
    nu[Y_] = acos(y_arg)/(2.0*M_PI);
    if (M1[2][3] < 0.0) nu[Y_] = 1.0 - nu[Y_];
  } else {
    globval.stable = false; nu[Y_] = -1.0;
    printf("GetNu: unstable (vertical)\n");
    return;
  }

  return;
}
#undef n


/* implementation */

/****************************************************************************/
/* void Cell_GetABGN(Matrix &M_, Vector2 &alpha, Vector2 &beta, Vector2 &gamma, Vector2 &nu)

   Purpose: called by Ring_Twiss_M and Ring_Twiss

      Get Twiss parameters from the transfer matrix M 

                             [Nx   ]
                         M = [     ]
                             [   Ny]

      where, in the case of mid-plane symmetry

             [ cos(mu) + alpha sin(mu)        beta sin(mu)      ]
         N = [                                                  ]
             [    -gamma sin(mu)        cos(mu) - alpha sin(mu) ]

        cos(mu) =  Trace(N)/2
        Alpha   =  (N11-N22)/(2*sin(mu))
        beta    =  N12/sin(mu)
        gamma   = -N21/sin(mu)

   Input:
       M oneturn matrix

   Output:
       alpha, beta, gamma vectors of the Twiss parameters
       nu tune vector

   Return:
       global.stable         
       global value to check the linear matrix is stable or not,
       true  stable, 
       false unstable

   Global variables:
       none

   Specific functions:
       getnu

   Comments:
       none

****************************************************************************/
void Cell_GetABGN(Matrix &M,
		  Vector2 &alpha, Vector2 &beta, Vector2 &gamma, Vector2 &nu)
{
  int     i = 0, j = 0;
  double  c = 0.0, s = 0.0;

  globval.stable = true;
  for (i = 0; i <= 1; i++) {
    j = (i+1)*2 - 1;
    /* c=cos(mu) = Tr(N)/2 */
    c = (M[j-1][j-1]+M[j][j])/2.0;
    globval.stable = (fabs(c) < 1.0);
    if (globval.stable) {
      // s = sin(mu)
      s = sqrt(1.0-c*c)*sgn(M[j-1][j]);
      alpha[i] = (M[j-1][j-1]-M[j][j])/(2.0*s); beta[i]  =  M[j-1][j]/s;
      gamma[i] = -(M[j][j-1]/s);
//      nu[i] = acos(c)/2/pi;
      GetNu(nu, M);
    }
  }
}


#define n  4
void Cell_Geteta(long i0, long i1, bool ring, double dP)
{
  long int  i = 0, lastpos = 0;
  int       j = 0, k = 0;
  Vector    xref;
  Vector    codbuf[Cell_nLocMax+1];
  CellType  *cellp;

  /* cod for the energy dP - globval.dPcommon / 2e0 */
  if (ring)
    GetCOD(globval.CODimax, globval.CODeps, dP-globval.dPcommon/2e0, lastpos);
  else { /* beam mode */
    CopyVec(n+2, globval.CODvect, xref); xref[4] = dP - globval.dPcommon/2e0;
    Cell_Pass(i0, i1, xref, lastpos);
  }

  /* Store chromatic orbit for elements i0 to i1 in codbuf */
  for (i = i0; i <= i1; i++)
    CopyVec(n+2, Cell[i].BeamPos, codbuf[i]);

  /* cod for the energy dP - globval.dPcommon / 2e0 */
  if (ring)
    GetCOD(globval.CODimax, globval.CODeps, dP+globval.dPcommon/2e0, lastpos);
  else { /* beam mode */
    CopyVec(n+2, globval.CODvect, xref); xref[4] = dP + globval.dPcommon/2e0;
    Cell_Pass(i0, i1, xref, lastpos);
  }

  /*       cod(dP-globval.dPcommon/2e0) - cod(dP-globval.dPcommon/2e0)     */
  /* eta = -----------------------------------------------------------     */
  /*                              globval.dPcommon                         */
  /*       cod'(dP-globval.dPcommon/2e0) - cod'(dP-globval.dPcommon/2e0)   */
  /* eta'= --------------------------------------------------------------- */
  /*                              globval.dPcommon                         */

  for (i = i0; i <= i1; i++) {
    cellp = &Cell[i];
    for (j = 1; j <= 2; j++) {
      k = j*2 - 1;
      cellp->Eta[j-1] = (cellp->BeamPos[k-1]-codbuf[i][k-1])/globval.dPcommon;
      cellp->Etap[j-1] = (cellp->BeamPos[k]-codbuf[i][k])/globval.dPcommon;
    }
  }
}

#undef n

/****************************************************************************/
/* void getprm(Matrix &Ascr, Vector2 &alpha, Vector2 &beta)

   Purpose:
     Computes Twiss parameters alpha and beta from the matrix Ascr
       
       M oneturn matrix    (A and M are symplectic)
                  
           ( A11  A12)         -1                 ( cos(mu) sin(mu))
       A = (         )   M = (A   R  A)  with R = (                )
           ( A21  A22)      2    2                (-sin(mu) cos(mu))
                    -1       x + px
       eps = 2*J o a     J = ------
                               2
                 2     2   2                                  2     2    2
       eps = (A22 + A21 ) x  + 2(-A22*A12-A21*A11) x*px + (A11 + A12 ) px
                gamma               alpha                     beta
                                        
   Input:                               
       A matrix

   Output:
       alpha vector
       beta  vector

   Return:
       none

   Global variables:
       none

   Specific functions:
       none

   Comments:
       none

****************************************************************************/
#define n 4
void getprm(Matrix &Ascr, Vector2 &alpha, Vector2 &beta)
{
  int  i, j;

  for (i = 1; i <= 2; i++) {
    j = i*2 - 1;
    alpha[i-1] = -(Ascr[j-1][j-1]*Ascr[j][j-1] + Ascr[j-1][j]*Ascr[j][j]);
    beta[i-1] = sqr(Ascr[j-1][j-1]) + sqr(Ascr[j-1][j]);
  }
}

/****************************************************************************/
/* void Cell_Twiss_M(long i0, long i1, Matrix &Ascr, bool chroma,
                   bool ring, double dP)

   Purpose: called by Ring_Twiss_M
       Calculate Twiss parameters from element i0 to element i1
       Method: extended matrix formalism
       
   Input:
       i0     first element
       i1     last element
       ring   true if a ring
       chroma true if compute chromaticities
       dP     energy offset
       
   Output:
       none

   Return:
       none

   Global variables:
       none

   Specific functions:
       getprm,
       CopyMat, CopyVec
       Elem_Pass_M, GetAngle
       
   Comments:
       17/07/03 use of M_PI instead of pi

****************************************************************************/
void Cell_Twiss_M(long i0, long i1, Matrix &Ascr, bool chroma,
		  bool ring, double dP)
{
  long int  i;
  int       j, k;
  Vector2   nu1, dnu;
  Vector    xref;
  Matrix    Ascr0, Ascr1;
  CellType  *cellp;

  if (dP != globval.dPparticle) Cell_SetdP(dP);

  /* Init */  
  for (j = 0; j <= 1; j++)
    nu1[j] = 0.0;

  /* get alpha beta for i0 */  
  cellp = &Cell[i0];
  getprm(Ascr, cellp->Alpha, cellp->Beta);
  memcpy(cellp->Nu, nu1, sizeof(Vector2));
  CopyMat(n+1, Ascr, Ascr0); CopyVec(n+2L, globval.CODvect, xref);

  for (i = i0+1; i <= i1; i++){
    CopyMat(n+1, Ascr0, Ascr1);
    /* Ascr1=Elem_M*Ascr0 */
    /* xref =Elem(xref)   */
    Elem_Pass_M(i, xref, Ascr1); 

    cellp = &Cell[i];
    /* get alpha and beta for element i */
    getprm(Ascr1, cellp->Alpha, cellp->Beta);

    for (j = 0; j <= 1; j++) {
      k = (j+1) * 2 - 1;
      /* get phase advances */
      dnu[j] = (GetAngle(Ascr1[k-1][k-1], Ascr1[k-1][k]) -
                GetAngle(Ascr0[k-1][k-1], Ascr0[k-1][k]))/(2.0*M_PI);
      if ((cellp->Elem.PL >= 0.0) && (dnu[j] < -1e-16))
	dnu[j] += 1.0;
      else if ((cellp->Elem.PL < 0.0) && (dnu[j] > 1e-16))
	dnu[j] -= 1.0;
      nu1[j] += dnu[j]; cellp->Nu[j] = nu1[j];
      /* Only correct for bare lattice */
      cellp->Eta[j] = Ascr1[k-1][4]; cellp->Etap[j] = Ascr1[k][4];
    }
    CopyMat(n+1, Ascr1, Ascr0);
  }
  if (chroma) Cell_Geteta(i0, i1, ring, dP);
}
#undef n

/****************************************************************************/
/* void dagetprm(ss_vect<tps> &Ascr, Vector2 &alpha, Vector2 &beta)

   Purpose: called by Cell_Twiss
     Compute Twiss parameters alpha and beta from the matrix Ascr

     M oneturn matrix    (A and M are symplectic)

         ( A11  A12 )         -1                 (  cos(mu) sin(mu) )
     A = (          )   M = (A   R  A)  with R = (                  )
         ( A21  A22 )                            ( -sin(mu) cos(mu) )

                            2    2
                  -1       x + px
     eps = 2*J o a     J = ------
                             2
               2     2   2                                  2     2    2
     eps = (A22 + A21 ) x  + 2(-A22*A12-A21*A11) x*px + (A11 + A12 ) px
              gamma               alpha                     beta

     A = Asrc
       
     alpha(i-1) = -(A(2i-1,2i-1)*A(2i,2i-1) + A(2i-1,2i)*A(2i,2i))
     beta(i-1)  =   A(2i-1,2i-1)*A(2i-1,2i-1) + A(2i-1,2i)*A(2i-1,2i)
       
   Input:
       Ascr

   Output:
       alpha alpha function vector
       beta  beta  function vector

   Return:
       none

   Global variables:
       none

   Specific functions:
       getmat

   Comments:
       none

****************************************************************************/
#define n 4
void dagetprm(ss_vect<tps> &Ascr, Vector2 &alpha, Vector2 &beta)
{
  int  i = 0, j = 0;

  for (i = 1; i <= 2; i++) {
    j = i*2 - 1;
    alpha[i-1] = -(getmat(Ascr, j, j)*getmat(Ascr, j+1, j)
                 + getmat(Ascr, j, j+1)*getmat(Ascr, j+1, j+1));
    beta[i-1] = sqr(getmat(Ascr, j, j)) + sqr(getmat(Ascr, j, j+1));
  }
}


void Cell_Twiss(long i0, long i1, ss_vect<tps> &Ascr, bool chroma, bool ring,
		double dP)
{
  long int      i = 0;
  int           j = 0, k = 0;
  Vector2       nu1, dnu;   /* absolute and relative phase advance */
  ss_vect<tps>  Ascr0, Ascr1;
  CellType      *cellp;

  /* initialization */
  for (j = 0; j <= 1; j++) 
  {
    nu1[j] = 0.0; 
    dnu[j] = 0.0; 
  }

  if (globval.radiation) globval.dE = 0.0;

  cellp = &Cell[i0];
  dagetprm(Ascr, cellp->Alpha, cellp->Beta);
  memcpy(cellp->Nu, nu1, sizeof(Vector2));

  Ascr0 = Ascr;
  for (j = 0; j <= n+1; j++)
    Ascr0[j] += tps(globval.CODvect[j]);

  Ascr1 = Ascr0;
  for (i = i0; i <= i1; i++) 
  {
    Elem_Pass(i, Ascr1); 
    cellp = &Cell[i];
    dagetprm(Ascr1, cellp->Alpha, cellp->Beta);
    for (j = 0; j <= 1; j++) {
      k = (j+1)*2 - 1;
      dnu[j] = (GetAngle(getmat(Ascr1, k, k), getmat(Ascr1, k, k+1)) -
      GetAngle(getmat(Ascr0, k, k), getmat(Ascr0, k, k+1)))/(2.0*M_PI);

      if ((cellp->Elem.PL >= 0.0) && (dnu[j] < -1e-16))
	dnu[j] += 1.0;
      else if ((cellp->Elem.PL < 0.0) && (dnu[j] > 1e-16))
	dnu[j] -= 1.0;

      nu1[j] += dnu[j];

      cellp->Nu[j] = nu1[j];
      cellp->Eta[j] = getmat(Ascr1, k, 5)*getmat(Ascr1, 6, 6) -
                     getmat(Ascr1, k, 6)*getmat(Ascr1, 6, 5);
      cellp->Etap[j] = getmat(Ascr1, k+1, 5)*getmat(Ascr1, 6, 6) -
                      getmat(Ascr1, k+1, 6)*getmat(Ascr1, 6, 5);
    }
    Ascr0 = Ascr1;
  }

  if (chroma && !globval.Cavity_on) Cell_Geteta(i0, i1, ring, dP);
}

#undef n


/****************************************************************************/
/* void Ring_Getchrom(double dP)

   Purpose: called by Ring_Twiss_M and Ring_Twiss
       Compute chromaticites of the ring by numerical differentiation
       and by evaluating each time the closed orbit

               nu(dP+dPlocal)-nu(dP-dPlocal)
           xi =-----------------------------
                       2 dPlocal     
                       
   Input:
       dP particle energy offset (should be zero cf comments)

   Output:
       none

   Return:
       none

   Global variables:
       status
       globval
       
   Specific functions:
       Cell_GetABGN, Cell_GetCOD

   Comments:
       03/01/03 Stability test from GETcod routines slightly modified
       WARNING : this routine does not give the chromaticities for dP != 0
                   but the local slope of the curve xi=f(dP)                   
       16/10/03 Modified convergence test: now done for both planes
       01/09/10 Modify the convergence criteria on relative diff of the chroma value
                The previous test does not work well for non zero chromaticities
                Test convergence changed if chroma closed to zero
****************************************************************************/
#define n               4
#define chromeps        1e-6 /* convergence condition for chromaticity computation */
#define LOG10 log(10.0)
#define ZEROCHRO 0.1

void Ring_Getchrom(double dP) {
	long int lastpos = 0;
	double NORMchro = 1.0;
	double dPlocal = 0.0, expo = 0.0, Norm_TEMP = 0.0;
	int j;
	Vector2 alpha = { 0.0, 0.0 }, beta = { 0.0, 0.0 }, gamma = { 0.0, 0.0 };
	Vector2 nu = { 0.0, 0.0 }, nu0 = { 0.0, 0.0 }, TEMP={0.0,0.0}, Chrom={0.0,0.0};;

	if (dP != 0.0)
		fprintf(stdout,
				"Ring_Getchrom: Warning this is NOT the CHROMA, dP=%e\n", dP);

	/* initialization */
	globval.Chrom[0] = 1e38;
	globval.Chrom[1] = 1e38;

	expo = log(globval.dPcommon) / LOG10;
	do {
		for (j = 0; j <= 1; j++)
			Chrom[j] = globval.Chrom[j];

		dPlocal = exp(expo * LOG10);
		//Get cod for energy dP - globval.dPcommon
		GetCOD(globval.CODimax, globval.CODeps, dP - dPlocal * 0.5, lastpos);

		if (!status.codflag) {
			// if no cod
			fprintf(stdout, "Ring_Getchrom:  Lattice is unstable for"
				" dP-globval.dPcommon=% .5e\n", dP - dPlocal * 0.5);
			return;
		}

		//get tunes for energy dP - globval.dPcommon/2 from oneturn matrix
		Cell_GetABGN(globval.OneTurnMat, alpha, beta, gamma, nu0);

		//Get cod for energy dP+globval.dPcommon
		GetCOD(globval.CODimax, globval.CODeps, dP + dPlocal * 0.5, lastpos);

		if (!status.codflag) { // if no cod
			fprintf(stdout, "Ring_Getchrom  Lattice is unstable for"
				" dP+globval.dPcommon=% .5e \n", dP + dPlocal * 0.5);
			return;
		}

		//get tunes for energy dP+globval.dPcommon/2 from one turn matrix
		Cell_GetABGN(globval.OneTurnMat, alpha, beta, gamma, nu);

		if (!globval.stable) {
			printf("Ring_Getchrom:  Lattice is unstable\n");
		}

		//Get chromaticities by numerical differentiation
		for (j = 0; j <= 1; j++)
			globval.Chrom[j] = (nu[j] - nu0[j]) / globval.dPcommon;

		/* Get chromaticities by numerical differentiation*/
		for (j = 0; j <= 1; j++) {
			globval.Chrom[j] = (nu[j] - nu0[j]) / dPlocal;
			TEMP[j] = fabs(globval.Chrom[j] - Chrom[j]);
		}

		Norm_TEMP = sqrt(TEMP[0] * TEMP[0] + TEMP[1] * TEMP[1]);
		NORMchro = sqrt(globval.Chrom[0] * globval.Chrom[0] + globval.Chrom[1]
				* globval.Chrom[1]);
		// if chroma closed to zero, norm is one for avoiding divergence of convergence test
		if (NORMchro < ZEROCHRO){
			NORMchro = 1.0;
		}

		// TEST CHROMA convergence
		if (trace) {
			fprintf(
					stdout,
					"\nexpo % e xix = % e xiz = % e Norm_TEMP = %e TEMPX %+e TEMPZ %+e\n",
					expo, Chrom[0], Chrom[1], Norm_TEMP, TEMP[0], TEMP[1]);
			fprintf(stdout, "expo % e nux = % e nuz = % e dPlocal= %+e\n",
					expo, nu0[0], nu0[1], dP - 0.5 * dPlocal);
			fprintf(stdout, "expo % e nux = % e nuz = % e dPlocal= %+e\n",
					expo, nu[0], nu[1], dP + 0.5 * dPlocal);
		}
		expo += 1.0;
		if (trace)
			fprintf(stdout, "%+e %+.12e %+.12e %+.12e %+.12e\n", dPlocal,
					globval.Chrom[0], fabs(globval.Chrom[0] - Chrom[0])
							/ Chrom[0], globval.Chrom[1], fabs(globval.Chrom[1]
							- Chrom[1]) / Chrom[1]);
	} while ((Norm_TEMP > chromeps * NORMchro) || (Norm_TEMP == 0e0));

	status.chromflag = true;
}

#undef n
#undef chromeps
#undef LOG10
#undef ZEROCHRO


/****************************************************************************/
/* static void Ring_Twiss_M(bool chroma, double dP)

   Purpose:  called by Ring_GetTwiss
       Calcule les parametres Twiss a l'energie dP
       si chroma=true, les chromaticites sont calculees
       Methode matricielle

       M : One turn transfer matrix

                                  [ cx  sx  .   . ]
                                  [               ]
                   -1             [-sx  cx  .   . ]
            M  -> A   M A = R =   [               ]
                                  [ .   .   cy  sy]
                                  [               ]
                                  [ .   .  -sy  cy]



   Input:
       bool true if chromaticities and dispersion to compute
               else false
       dP      particle energy offset  

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
#define n 4
void Ring_Twiss_M(bool chroma, double dP)
{
  long int  lastpos = 0;
  int       j;
  Vector2   alpha={0.0,0.0}, beta={0.0,0.0}, gamma={0.0,0.0}, nu={0.0,0.0};
  Vector    eta0;
  Matrix    R, C, Ascr;

  /* Get closed orbit and compute oneturn matrix */
  GetCOD(globval.CODimax, globval.CODeps, dP, lastpos);

  if (!status.codflag) /* Check if stable */
    return;
  /* compute twiss parameter using oneturn matrix */
  Cell_GetABGN(globval.OneTurnMat, alpha, beta, gamma, nu);

   /* Get eigenvalues and eigenvectors for the one turn transfer matrix */
  GDiag((long)n, Cell[globval.Cell_nLoc].S, globval.Ascr, globval.Ascrinv, R,
        globval.OneTurnMat, globval.Omega, globval.Alphac);

  /* Only correct for bare lattice */
  for (j = 0; j <= n; j++)
    eta0[j] = globval.OneTurnMat[j][n];

  UnitMat((long)n, C); SubMat((long)n, globval.OneTurnMat, C);

  if (!InvMat((long)n, C))
    printf("** matrix is singular\n");

  LinTrans((long)n, C, eta0);

  for (j = 0; j <= n; j++) {
    globval.Ascr[n][j] = 0.0; 
    globval.Ascr[j][n] = eta0[j];
  }

  CopyMat(n+1, globval.Ascr, Ascr);
  Cell_Twiss_M(0, globval.Cell_nLoc, Ascr, chroma, true, dP);

  /* Copies tunes into global variable BUG !!!*/
  memcpy(globval.TotalTune, Cell[globval.Cell_nLoc].Nu, sizeof(Vector2));
  status.tuneflag = true;

  if (chroma)
  { /* compute chromaticities */
    Ring_Getchrom(dP);
    GetCOD(globval.CODimax, globval.CODeps, dP, lastpos);
  }
}

#undef n


/****************************************************************************/
/* void Ring_Twiss(bool chroma, double dP)

   Purpose:  called by Ring_GetTwiss
       Computes twiss parameters for an energy offset dP using da method
           First call GetCOD() to check whether can find a COD or not, if yes,
           check the linear one turn matrix is stable or not, then put the linear
	   one turn map

   Input:
       chroma if true computes chromaticities as well
       dP     energy offset

   Output:
       none

   Return:
       none

   Global variables:
       globval
       status

   Specific functions:
       none

   Comments:
       

****************************************************************************/
void Ring_Twiss(bool chroma, double dP)
{
  long int      lastpos = 0;
  int           n = 0;
  Vector2       alpha={0.0, 0.0}, beta={0.0, 0.0};
  Vector2       gamma={0.0, 0.0}, nu={0.0, 0.0};
  Matrix        R;
  ss_vect<tps>  AScr; /* DA map*/

  if (globval.Cavity_on)
    n = 6L;  /* 6D tracking */
  else      /* 4D tracking */
    n = 4L;
    
  if(trace)
  printf("\n %d D tracking....\n",n);
    
/* Gets closed orbit and computes oneturn map around it*/
  GetCOD(globval.CODimax, globval.CODeps, dP, lastpos);

  if (!status.codflag) 
    return;

  // Check if stable
  Cell_GetABGN(globval.OneTurnMat, alpha, beta, gamma, nu);
  // Get eigenvalues and eigenvectors for the one turn transfer matrix
  GDiag(n, Cell[globval.Cell_nLoc].S, globval.Ascr, globval.Ascrinv, R,
        globval.OneTurnMat, globval.Omega, globval.Alphac);
  // Puts zeroes in constant part of da map
  putlinmat(n, globval.Ascr, AScr);
  
  if (!globval.Cavity_on) 
  {
    AScr[delta_] = 0.0; 
    AScr[ct_] = 0.0;
  }

  Cell_Twiss(0, globval.Cell_nLoc, AScr, chroma, true, dP);

  memcpy(globval.TotalTune, Cell[globval.Cell_nLoc].Nu, sizeof(Vector2));
  status.tuneflag = true;

  if (chroma && !globval.Cavity_on) {
    Ring_Getchrom(dP); 
    GetCOD(globval.CODimax, globval.CODeps, dP, lastpos);
  }
}

/****************************************************************************/
/* void Ring_GetTwiss(bool chroma, double dP)

   Purpose:
       Computes Twiss functions w/ or w/o chromaticities
       for particle of energy offset dP
       matrix or DA method
       
       Also get the momentum compact factor based on the definition:
       Alphac = delta_C/delta*1/C

   Input:
       Chroma if true compute chromaticities and dispersion
              if false dispersion is set to zero !!!
       Dp energy offset

   Output:
       none

   Return:
       none

   Global variables:
       globval, trace

   Specific functions:
       Ring_Twiss_M, Ring_Twiss

   Comments:
       none

****************************************************************************/
void Ring_GetTwiss(bool chroma, double dP)
{

  if (trace) printf("enter ring_gettwiss\n");
  
  if (globval.MatMeth)  /* matrix method */
    Ring_Twiss_M(chroma, dP);
  else /* da method */
    Ring_Twiss(chroma, dP);
    
  globval.Alphac = globval.OneTurnMat[ct_][delta_]/Cell[globval.Cell_nLoc].S;
  if (trace) printf("exit ring_gettwiss\n");
}


/* Local variables for Ring_Fittune: */
struct LOC_Ring_Fittune
{
  jmp_buf _JL999;
};


/****************************************************************************/
/* void TransTrace(long i0, long i1, Vector2 &alpha, Vector2 &beta, Vector2 &eta,
                   Vector2 &etap, double *codvect)

   Purpose:  TransTwiss
      Compute Twiss fonction for a tranfert line from element i0 to element i1

   Input:
       alpha   alpha fonctions at the line entrance
       beta    beta fonctions at the line entrance
       eta     disperion fonctions at the line entrance
       etap    dipersion derivatives fonctions at the line entrance
       codvect closed orbit fonctions at the line entrance

   Output:
       none

   Return:
       none

   Global variables:
       globval, trace

   Specific functions:
       Cell_Twiss_M, Cell_Pass_M

   Comments:
       28/10/03 phase advances added

****************************************************************************/
void TransTrace(long i0, long i1, Vector2 &alpha, Vector2 &beta, Vector2 &eta,
                Vector2 &etap, Vector &codvect)
{
  long i, j, lastpos;
  double sb;
  Matrix Ascr;

  UnitMat(6L, Ascr);
  for (i = 1; i <= 2L; i++) {
    sb = sqrt(beta[i-1]); j = i*2 - 1;
    Ascr[j-1][j-1] = sb;             Ascr[j-1][j] = 0.0;
    Ascr[j][j-1]   = -(alpha[i-1]/sb); Ascr[j][j] = 1/sb;
  }
  Ascr[0][4] = eta[0]; Ascr[1][4] = etap[0];
  Ascr[2][4] = eta[1]; Ascr[3][4] = etap[1];

  for (i = 0; i < ss_dim; i++) {
    globval.CODvect[i] = codvect[i];
  }

  // get twiss parameter for each element
  Cell_Twiss_M(i0, i1, Ascr, false, false, codvect[4]);
  memcpy(globval.TotalTune, Cell[globval.Cell_nLoc].Nu, sizeof(Vector2));
  status.tuneflag = true;

  for (i = 0; i < ss_dim; i++) {
    globval.CODvect[i] = codvect[i];
  }

  // compute oneturn matrix
  UnitMat(5L, globval.OneTurnMat);
  Cell_Pass_M(0, globval.Cell_nLoc, globval.CODvect, globval.OneTurnMat,
              lastpos);
}

/****************************************************************************/
/* void shiftk(long Elnum, double dk, struct LOC_Ring_Fittune *LINK)

   Purpose:


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
       none

****************************************************************************/
#define dP              0.0
void shiftk(long Elnum, double dk, struct LOC_Ring_Fittune *LINK)
{
  CellType   *cellp;
  elemtype   *elemp;
  MpoleType  *M;

  cellp = &Cell[Elnum]; elemp = &cellp->Elem; M = elemp->M;
  M->PBpar[Quad+HOMmax] += dk;
  Mpole_SetPB(cellp->Fnum, cellp->Knum, (long)Quad);
}


/****************************************************************************/
/* void checkifstable(struct LOC_Ring_Fittune *LINK)

   Purpose:


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
       none

****************************************************************************/
void checkifstable(struct LOC_Ring_Fittune *LINK)
{
  if (!globval.stable) {
    printf("  lattice is unstable\n");
    longjmp(LINK->_JL999, 1);
  }
}


/****************************************************************************/
/* void Ring_Fittune(Vector2 &nu, double eps, long *nq, long *qf, long *qd,
                  double dkL, long imax)

   Purpose: called by fittune
       Fit tunes using two family of quadrupoles
       Linear method

   Input:
       nu  target tunes
       eps precision
       nq  number of quad of family qf and qd
       qf  position of qf magnets
       qd  position of qd magnets
       dKL variation on strengths
       imax maximum number of iteration

   Output:
       none

   Return:
       none

   Global variables:
       none

   Specific functions:
       none

   Comments:
       17 mars 2004: removed labels

****************************************************************************/
void Ring_Fittune(Vector2 &nu, double eps, iVector2 &nq, long qf[], long qd[],
                  double dkL, long imax)
{
  struct LOC_Ring_Fittune V;

  int      i, j, k;
  Vector2  nu0, nu1;
  Vector  dkL1, dnu;
  Matrix A;
  bool prt = false;

  if (setjmp(V._JL999)) return;

  if (trace)
    printf("  Tune fit, nux =%10.5f, nuy =%10.5f, eps =% .3E,"
	   " imax =%4ld, dkL = % .5E\n", nu[0], nu[1], eps, imax, dkL);
  Ring_GetTwiss(false, dP); checkifstable(&V);
  memcpy(nu0, globval.TotalTune, sizeof(Vector2));
  i = 0;
  do {
    i++;
    /* First vary kf then kd */
    for (j = 1; j <= 2L; j++) {
      for (k = 0; k < nq[j-1]; k++) {
        if (j == 1)
          shiftk(qf[k], dkL, &V); // new value for qf
        else
          shiftk(qd[k], dkL, &V); // new value for qd
      }
      Ring_GetTwiss(false, dP);
      nu1[0] = globval.TotalTune[0]; nu1[1] = globval.TotalTune[1];
      checkifstable(&V);
      for (k = 0; k <= 1; k++) {
        dnu[k] = nu1[k] - (long)nu1[k] - nu0[k] + (long)nu0[k];
        if (fabs(dnu[k]) > 0.5) dnu[k] = 1 - fabs(dnu[k]);
        A[k][j-1] = dnu[k]/dkL;
      }

      /* Restore strength */
      for (k = 0; k < nq[j-1]; k++) {
        if (j == 1)
          shiftk(qf[k], -dkL, &V);
        else
          shiftk(qd[k], -dkL, &V);
        }
      }

    if (!InvMat(2L, A)) {
      printf("  A is singular\n");
      return;
    }

    for (j = 0; j <= 1; j++)
      dkL1[j] = nu[j] - nu0[j];
    LinTrans(2L, A, dkL1);
    for (j = 1; j <= 2; j++) {
      for (k = 0; k < nq[j-1]; k++) {
	if (j == 1)
	  shiftk(qf[k], dkL1[j-1], &V);
	else
	  shiftk(qd[k], dkL1[j-1], &V);
      }
    }
    Ring_GetTwiss(false, dP); checkifstable(&V);
    memcpy(nu0, globval.TotalTune, sizeof(Vector2));
    if (trace)
      printf("  Nux = %10.6f(%10.6f), Nuy = %10.6f(%10.6f),"
	     " QFam1*L = % .5E, QFam2*L = % .5E @%3d\n",
	     nu0[0], nu1[0], nu0[1], nu1[1],
	     Elem_GetKval(Cell[qf[0]].Fnum, 1, (long)Quad),
	     Elem_GetKval(Cell[qd[0]].Fnum, 1, (long)Quad), i);
  } while (sqrt(sqr(nu[0]-nu0[0])+sqr(nu[1]-nu0[1])) >= eps && i != imax);
  if (prt)
	  // print new K-value for 1s element of the family
    printf("Reached  Nux = %10.6f, Nuy = %10.6f,"
	     " QFam1*L = % .5E, QFam2*L = % .5E @iteration %3d\n",
	     nu[0], nu[1],
	     Elem_GetKval(Cell[qf[0]].Fnum, 1, (long)Quad),
	     Elem_GetKval(Cell[qd[0]].Fnum, 1, (long)Quad), i);
}
//#undef dP

/****************************************************************************/
/* void lnls_Ring_Fittune(Vector2 &nu, double eps, int n_quads, long nq[], long quad[],
		double dkL, long imax)

   Purpose: called by lnls_fittune
       Fit tunes using up to 5 families of quadrupoles
       SVD method

   Input:
       nu  target tunes
       eps precision
       n_quads  number of quad families
       nq  number of of kids for each quad family
       quad  matrix where each line contains the position of each kid of a quad family
       dKL variation on strengths for SVD calculation
       imax maximum number of iteration

   Output:
       none

   Return:
       none

   Global variables:
       none

   Specific functions:
       none

   Comments:
       03 september 2012: creation

****************************************************************************/
void lnls_Ring_Fittune(Vector2 &nu, double eps, int n_quads, long nq[], long quad[],
		double dkL, long imax)
{
	struct LOC_Ring_Fittune V;

	int      i, j, k;
	double nu0[2], nu1[2], dnumin, Kmin[5], K[5];
	double **U, *w, **Vi, *dnu, *dnu1, *dkL1;

	bool prt = false;

	dnu = dvector(1, 2);
	dnu1 = dvector(1, 2);
	dkL1 = dvector(1, n_quads);
	U = dmatrix(1, 2, 1, n_quads);
	w = dvector(1, n_quads);
	Vi = dmatrix(1, n_quads, 1, n_quads);

	if (setjmp(V._JL999)) return;

	if (trace)
		printf("  Tune fit, nux =%10.5f, nuy =%10.5f, eps =% .3E,"
				" imax =%4ld, dkL = % .5E\n", nu[0], nu[1], eps, imax, dkL);
	Ring_GetTwiss(false, dP); checkifstable(&V);
	nu0[0] = globval.TotalTune[0]; nu0[1] = globval.TotalTune[1];
// calc parameters before loop 2013-03-07
	for (j = 0; j <= 1; j++)
		dnu[j+1] = nu[j] - nu0[j];
//11/02/2014	dnu[j+1] = nu[j] - (long)nu[j] - nu0[j] + (long)nu0[j];

	dnumin = sqrt(sqr(dnu[1])+sqr(dnu[2]));
	for(j = 1; j <= n_quads; j++) {
		Kmin[j-1] = Elem_GetKval(Cell[quad[(j-1)*fitvectmax]].Fnum, 1, (long)Quad);
	}

	i = 0;
	do {
		i++;
		/* First vary kf then kd */
		for (j = 1; j <= n_quads; j++) {
			for (k = 1; k <= nq[j-1]; k++) {
				shiftk(quad[(j-1)*fitvectmax + k-1], dkL, &V); // new value for quads
			}
			Ring_GetTwiss(false, dP);
			nu1[0] = globval.TotalTune[0]; nu1[1] = globval.TotalTune[1];
			checkifstable(&V);
			for (k = 0; k <= 1; k++) {
				dnu1[k+1] = nu1[k] - nu0[k];
//11/02/2014	dnu1[k+1] = nu1[k] - (long)nu1[k] - nu0[k] + (long)nu0[k];
				if (fabs(dnu1[k+1]) > 0.5) dnu1[k+1] = 1 - fabs(dnu1[k+1]);
				U[k+1][j] = dnu1[k+1]/dkL;
			}

			/* Restore strength */
			for (k = 1; k <= nq[j-1]; k++)
				shiftk(quad[(j-1)*fitvectmax + k-1], -dkL, &V);
		}


		dsvdcmp(U, 2, n_quads, w, Vi);

		for (j = 1; j <= n_quads;j++) if (w[j]<eps) w[j] = 0;

		dsvbksb(U, w, Vi, 2, n_quads, dnu, dkL1);

		for (j = 1; j <= n_quads; j++) {
			for (k = 1; k <= nq[j-1]; k++)
				shiftk(quad[(j-1)*fitvectmax + k-1], dkL1[j], &V);
		}
		Ring_GetTwiss(false, dP); checkifstable(&V);
		nu0[0] = globval.TotalTune[0]; nu0[1] = globval.TotalTune[1];

		for (j = 0; j <= 1; j++)
			dnu[j+1] = nu[j] - nu0[j];
//11/02/2014 dnu[j+1] = nu[j] - (long)nu[j] - nu0[j] + (long)nu0[j];
// keep the best result so far 2013-03-07, Fernando.
		if ( sqrt(sqr(dnu[1])+sqr(dnu[2])) <= dnumin )
		{
			dnumin = sqrt(sqr(dnu[1])+sqr(dnu[2]));
			for(j = 1; j <= n_quads; j++)
			{
				Kmin[j-1] = Cell[quad[(j-1)*fitvectmax]].Elem.M->PB[HOMmax + 2];
			}
		}

		if (trace){
			printf("  Nux = %10.6f(%10.6f), Nuy = %10.6f(%10.6f),",
					nu0[0], nu[0], nu0[1], nu[1]);
			for(j = 1; j <= n_quads; j++) {
				printf(" QFam%d*L = % .5E, ", j,	Elem_GetKval(Cell[quad[(j-1)*fitvectmax]].Fnum, 1, (long)Quad));
			}
			printf(", %d", i);
		}

	} while (sqrt(sqr(dnu[1])+sqr(dnu[2])) >= eps && i != imax);
// If the max number of iteractions is reached, set the best aproximation, 2013-03-07, Fernando.
	if (i == imax)
	{
		printf("FitTune did not converge\n");
		for(j = 1; j <= n_quads; j++) {
			K[j-1] = Cell[quad[(j-1)*fitvectmax]].Elem.M->PB[HOMmax + 2];
		}	
		for (j = 1; j <= n_quads; j++) {
			for (k = 1; k <= nq[j-1]; k++)
				shiftk(quad[(j-1)*fitvectmax + k-1], (Kmin[j-1]-K[j-1]), &V);
		}	
	}

	if (prt){
		// print new K-value for 1s element of the family
		printf("Reached  Nux = %10.6f, Nuy = %10.6f,",
				nu0[0], nu0[1]);
		for(j = 1; j <= n_quads; j++) {
			printf(" QFam%d*L = % .5E, ", j,	Elem_GetKval(Cell[quad[(j-1)*fitvectmax]].Fnum, 1, (long)Quad));
		}
		printf(", %d", i);
	}

	free_dvector(dnu,1, 2);
	free_dvector(dnu1,1, 2);
	free_dvector(dkL1,1, n_quads);
	free_dmatrix(U, 1, 2, 1, n_quads);
	free_dvector(w, 1, n_quads);
	free_dmatrix(Vi,1, n_quads, 1, n_quads);

}
#undef dP


#define dP  0.0
void shiftkp(long Elnum, double dkp)
{
  CellType  *cellp;
  elemtype  *elemp;
  MpoleType *M;

  cellp = &Cell[Elnum]; elemp = &cellp->Elem; M = elemp->M;
  M->PBpar[Sext+HOMmax] += dkp;
  Mpole_SetPB(cellp->Fnum, cellp->Knum, (long)Sext);
}


void Ring_Fitchrom(Vector2 &ksi, double eps, iVector2 &ns,
		   long sf[], long sd[], double dkpL, long imax)
{
  bool      rad;
  long int  lastpos;
  int       i, j, k;
  Vector2   ksi0;
  Vector    dkpL1, dksi;
  Matrix    A;

  if (trace)
    printf("  Chromaticity fit, ksix =%10.5f, ksiy =%10.5f, eps =% .3E"
	   ", imax =%4ld, dkpL =%10.5f\n", ksi[0], ksi[1], eps, imax, dkpL);

  /* Turn off radiation */
  rad = globval.radiation; globval.radiation = false;
  GetCOD(globval.CODimax, globval.CODeps, dP, lastpos); Ring_Getchrom(dP);
  for (j = 0; j <= 1; j++)
    ksi0[j] = globval.Chrom[j];
  i = 0;
  do {
    i++;
    /* First vary sf then sd */
    for (j = 1; j <= 2; j++) {
      for (k = 0; k < ns[j-1]; k++) {
	if (j == 1)
	  shiftkp(sf[k], dkpL);
	else
	  shiftkp(sd[k], dkpL);
      }
      GetCOD(globval.CODimax, globval.CODeps, dP, lastpos); Ring_Getchrom(dP);
      for (k = 0; k <= 1; k++) {
	dksi[k] = globval.Chrom[k] - ksi0[k];
	A[k][j-1] = dksi[k] / dkpL;
      }
      /* Restore strength */
      for (k = 0; k < ns[j-1]; k++) {
	if (j == 1)
	  shiftkp(sf[k], -dkpL);
	else
	  shiftkp(sd[k], -dkpL);
      }
    }
    if (!InvMat(2L, A)) {
      printf("  A is singular\n");
      goto _L999;
    }
    for (j = 0; j <= 1; j++)
      dkpL1[j] = ksi[j] - ksi0[j];
    LinTrans(2L, A, dkpL1);
    for (j = 1; j <= 2; j++) {
      for (k = 0; k < ns[j-1]; k++) {
	if (j == 1)
	  shiftkp(sf[k], dkpL1[j-1]);
	else
	  shiftkp(sd[k], dkpL1[j-1]);
      }
    }
    GetCOD(globval.CODimax, globval.CODeps, dP, lastpos); Ring_Getchrom(dP);
    for (j = 0; j <= 1; j++)
      ksi0[j] = globval.Chrom[j];
    if (trace)
      printf("  ksix =%10.6f, ksiy =%10.6f, SF = % .5E, SD = % .5E @%3d\n",
	     ksi0[0], ksi0[1], Elem_GetKval(Cell[sf[0]].Fnum, 1, (long)Sext),
	     Elem_GetKval(Cell[sd[0]].Fnum, 1, (long)Sext), i);
  } while (sqrt(sqr(ksi[0]-ksi0[0])+sqr(ksi[1]-ksi0[1])) >= eps && i != imax);
_L999:
  /* Restore radiation */
  globval.radiation = rad;
}

#undef dP


#define dP 0.0


/* Local variables for Ring_FitDisp: */
struct LOC_Ring_FitDisp
{
  jmp_buf _JL999;
};


static void shiftk_(long Elnum, double dk, struct LOC_Ring_FitDisp *LINK)
{
  CellType *cellp;
  elemtype *elemp;
  MpoleType *M;

  cellp = &Cell[Elnum];
  elemp = &cellp->Elem;
  M = elemp->M;
  M->PBpar[Quad+HOMmax] += dk;
  Mpole_SetPB(cellp->Fnum, cellp->Knum, (long)Quad);
}


void checkifstable_(struct LOC_Ring_FitDisp *LINK)
{
  if (!globval.stable) {
    printf("  lattice is unstable\n");
    longjmp(LINK->_JL999, 1);
  }
}


void Ring_FitDisp(long pos, double eta, double eps, long nq, long q[],
                  double dkL, long imax)
{
  /*pos : integer; eta, eps : double;
                         nq : integer; var q : fitvect;
                         dkL : double; imax : integer*/
  struct LOC_Ring_FitDisp V;

  int     i, j;
  double  dkL1, Eta0, deta;
  bool    rad = false;

  if (setjmp(V._JL999)) goto _L999;

  if (trace)
    printf("  Dispersion fit, etax =%10.5f, eps =% .3E"
	   ", imax =%4ld, dkL =%10.5f\n",
	   eta, eps, imax, dkL);
  /* Turn off radiation */
  rad = globval.radiation; globval.radiation = false;
  Ring_GetTwiss(true, dP); checkifstable_(&V);
  Eta0 = Cell[pos].Eta[0];
  i = 0;
  while (fabs(eta-Eta0) > eps && i < imax) {
    i++;
    for (j = 0; j < nq; j++)
      shiftk_(q[j], dkL, &V);
    Ring_GetTwiss(true, dP); checkifstable_(&V);
    deta = Cell[pos].Eta[0] - Eta0;
    if (deta == 0.0) {
      printf("  deta is 0\n");
      goto _L999;
    }
    dkL1 = (eta-Eta0)*dkL/deta - dkL;
    for (j = 0; j < nq; j++)
      shiftk_(q[j], dkL1, &V);
    Ring_GetTwiss(true, dP); checkifstable_(&V);
    Eta0 = Cell[pos].Eta[0];
    if (trace)
      printf("  Dispersion = % .5E, kL =% .5E @%3d\n",
	     Eta0, Elem_GetKval(Cell[q[0]].Fnum, 1, (long)Quad), i);
  }
_L999:
  /* Restore radiation */
  globval.radiation = rad;
}
#undef dP

/* End. */
