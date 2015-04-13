/* Tracy-2

   J. Bengtsson  CBP, LBL      1990 - 1994   Pascal version
                 SLS, PSI      1995 - 1997
   M. Boege      SLS, PSI      1998          C translation
   L. Nadolski   SOLEIL        2002          Link to NAFF, Radia field maps
   J. Bengtsson  NSLS-II, BNL  2004 -        

  eigenv.c -- Eigenvalue routines

*/

#include "tracy_lib.h"

/****************************************************************************/
/* long closest(double x, double x1, double x2, double x3)

   Purpose:  called by geigen
        Compare x to x1, x2 and x3
        Return the i giving the closest xi from x

   Input:
       x  number to compare
       x1, x2, x3 numbers iused in compairison

   Output:
       integer 1, 2 or 3

   Return:
       none

   Global variables:
       none

   Specific functions:
       none

   Comments:
       none

****************************************************************************/
static int closest(double x, double x1, double x2, double x3)
{
  double dx1, dx2, dx3;

  dx1 = fabs(x - x1);
  dx2 = fabs(x - x2);
  dx3 = fabs(x - x3);
  if (dx1 < dx2 && dx1 < dx3)
    return 1;
  else if (dx2 < dx1 && dx2 < dx3)
    return 2;
  else
    return 3;
}

/****************************************************************************/
/* Local void SwapMat(int n, Matrix &A, int i, int j)

   Purpose: called by geigen
       A[*, i] <==> A[*, j]
       A[i, *] <==> A[j, *]  


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
static void SwapMat(int n, Matrix &A, int i, int j)
{
  double x;
  int k;

  for (k = 0; k < n; k++)
  {
    x = A[k][i - 1];
    A[k][i - 1] = A[k][j - 1];
    A[k][j - 1] = x;
  }
}

/****************************************************************************/
/* Local void Swap(double *x, double *y)

   Purpose:  called by geigen
       Swap x and y

   Input:
       x, y

   Output:
       x, y

   Return:
       none

   Global variables:
       none

   Specific functions:
       none

   Comments:
       none

****************************************************************************/
static void Swap(double *x, double *y)
{
  double z;

   z = *x;
  *x = *y;
  *y =  z;
}


/* Implementation */

/****************************************************************************/
/* static void geigen(int n, Matrix &fm, Matrix &Vre, Matrix &Vim,
                   Vector &wr, Vector &wi)

   Purpose:  called by GDiag
      This routine finds the eigenvalues and eigenvectors of the full matrix fm 

      Compute eigenvalues and eigenvectors using double
      precision Eispack routines:

      EISPACK is a collection of double-precision Fortran subroutines that
      compute the eigenvalues and eigenvectors of nine classes of matrices:
      complex general, complex Hermitian, real general, real symmetric, real
      symmetric banded, real symmetric tridiagonal, special real
      tridiagonal, generalized real, and generalized real symmetric matices.
      In addition, two routines are included that use singular value
      decomposition to solve certain least-squares problems.
      
   Input:
       n  matrix dimension
       fm input "full" matrix

   Output:
       Vre, Vim  real and imaginary part of the eigenvectors
       wr, wi    real and imaginary part of the eigenvalues

   Return:
       none

   Global variables:
       none

   Specific functions:
       ETY, ETYT, ety2

   Comments:
       30/12/02 label removed
       17/07/03 use M_PI instead of pi

****************************************************************************/
static void geigen(int n, Matrix &fm, Matrix &Vre, Matrix &Vim,
                   Vector &wr, Vector &wi)
{
  int info, i, j, k, c;
  Vector ort, cosfm, sinfm, nufm1, nufm2, nu1, nu2;
  Matrix aa, vv;
  double TEMP;

  /* copy matrix to temporary storage [the matrix aa is destroyed]*/
  for (i = 0; i < n; i++)
  {
    for (j = 0; j < n; j++)
      aa[i][j] = fm[i][j];
  }
  /* Compute eigenvalues and eigenvectors using double
  precision Eispack routines: */
  ETY(n, 1, n, aa, ort);
  ETYT(n, 1, n, aa, ort, vv);
  ety2(n, 1, n, aa, wr, wi, vv, info);
  if (info != 0)
  {
    printf("  Error in eig\n");
//    goto _L999;
    return;
  }
  for (i = 1; i <= n / 2; i++)
  {
    for (j = 0; j < n; j++)
    {
      Vre[j][i * 2 - 2] =  vv[j][i * 2 - 2];
      Vim[j][i * 2 - 2] =  vv[j][i * 2 - 1];
      Vre[j][i * 2 - 1] =  vv[j][i * 2 - 2];
      Vim[j][i * 2 - 1] = -vv[j][i * 2 - 1];
    }
  }
  for (i = 0; i < n / 2; i++)
  {
    j = (i + 1) * 2 - 1;
    cosfm[i] = (fm[j - 1][j - 1] + fm[j][j]) / 2;
    if (fabs(cosfm[i]) > (double)1.0)
    {
      printf("geigen: cosfm[%d]=% .13E > 1.0!\n", i + 1, cosfm[i]);
      globval.stable = false;
//      goto _L999;
      return;
    }
    TEMP     = cosfm[i];
    sinfm[i] = sgn(fm[j - 1][j]) * sqrt(1.0 - TEMP * TEMP);
    nufm1[i] = GetAngle(cosfm[i], sinfm[i]) / (2.0 * M_PI);
    if (nufm1[i] < 0.0)
      nufm1[i]++;
    if (nufm1[i] <= 0.5)
      nufm2[i] = nufm1[i];
    else
      nufm2[i] = 1.0 - nufm1[i];
    nu1[i] = GetAngle(wr[j - 1], wi[j - 1]) / (2.0 * M_PI);
    if (nu1[i] < 0.0)
      nu1[i]++;
    if (nu1[i] <= 0.5)
      nu2[i] = nu1[i];
    else
      nu2[i] = 1.0 - nu1[i];
    /*        writeln(' est. nu   :', nufm2[i], ', eigenvalue:', nu2[i]);*/
  }
  for (i = 1; i <= n / 2; i++)
  {
    c = closest(nufm2[i - 1], nu2[0], nu2[1], nu2[2]);
    if (c != i)
    {
      j = c * 2 - 1;
      k = i * 2 - 1;
      /*          writeln(' swapping ', j:0, ' with ', k:0);*/
      SwapMat(n, Vre, j, k);
      SwapMat(n, Vim, j, k);
      SwapMat(n, Vre, j + 1, k + 1);
      SwapMat(n, Vim, j + 1, k + 1);
      Swap(&wr[j - 1], &wr[k - 1]);
      Swap(&wi[j - 1], &wi[k - 1]);
      Swap(&wr[j], &wr[k]);
      Swap(&wi[j], &wi[k]);
      Swap(&nu1[i - 1], &nu1[c - 1]);
      Swap(&nu2[i - 1], &nu2[c - 1]);
    }
  }
  for (i = 1; i <= n / 2; i++)
  {
    if ((0.5 - nufm1[i - 1]) * (0.5 - nu1[i - 1]) < 0.0)
    {
      j = i * 2 - 1;
      SwapMat(n, Vim, j, j + 1);
      Swap(&wi[j - 1], &wi[j]);
    }
  }
//_L999: ;
}


/* Local variables for GDiag: */
struct LOC_GDiag {
  int     n;
  Matrix  *Ainv;
  Matrix  JJ, Vre, Vim;
} ;

/****************************************************************************/
/* void InitJJ(struct LOC_GDiag &LINK)

   Purpose: called by GDiag
       Initializes the symplectic matrix
                       (  0 1  0 0 )
        if n=4     J = ( -1 0  0 0 )
                       (  0 0  0 1 )
                       (  0 0 -1 0 )
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
       30/12/02 code simplified

****************************************************************************/
static void InitJJ(struct LOC_GDiag &LINK)
{
  int i, j, FORLIM, FORLIM1;

  FORLIM = FORLIM1 = LINK.n;
//  FORLIM = LINK.n;
  for (i = 0; i < FORLIM; i++)
  {
//    FORLIM1 = LINK.n;
    for (j = 0; j < FORLIM1; j++)
      LINK.JJ[i][j] = 0.0;
  }
//  FORLIM = LINK.n;
  for (j = 1; j <= FORLIM; j++)
  {
    if (j & 1)
    {
      LINK.JJ[j - 1][j] = 1.0;
      LINK.JJ[j][j - 1] = -1.0;
    }
  }
}

/****************************************************************************/
/* Local void GetAinv(struct LOC_GDiag &LINK)

   Purpose:  called by GDiag


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
static void GetAinv(struct LOC_GDiag &LINK)
{
  int i, j, k, sgn;
  double z;
  int FORLIM, FORLIM1, FORLIM2;

  FORLIM = LINK.n;
  for (i = 0; i < FORLIM; i++)
  {
    if ((i + 1) & 1)
    {
      z = 0.0;
      FORLIM1 = LINK.n;
      for (j = 0; j < FORLIM1; j++)
      {
        FORLIM2 = LINK.n;
        for (k = 0; k < FORLIM2; k++)
          z += LINK.Vre[j][i] * LINK.JJ[j][k] * LINK.Vim[k][i];
      }
      sgn = sgn(z);
      z = sqrt(fabs(1.0 / z));
      FORLIM1 = LINK.n;
      for (j = 0; j < FORLIM1; j++)
      {
        LINK.Vre[j][i] *= z;
        LINK.Vim[j][i] = sgn * LINK.Vim[j][i] * z;
      }
    }
  }
  FORLIM = LINK.n;
  for (i = 0; i < FORLIM; i++)
  {
    (*LINK.Ainv)[0][i] = sgn(LINK.Vre[0][0]) * LINK.Vre[i][0];
    (*LINK.Ainv)[1][i] = sgn(LINK.Vre[0][0]) * LINK.Vim[i][0];
    (*LINK.Ainv)[2][i] = sgn(LINK.Vre[2][2]) * LINK.Vre[i][2];
    (*LINK.Ainv)[3][i] = sgn(LINK.Vre[2][2]) * LINK.Vim[i][2];
    if (LINK.n > 4)
    {
      (*LINK.Ainv)[4][i] = sgn(LINK.Vre[4][4]) * LINK.Vre[i][4];
      (*LINK.Ainv)[5][i] = sgn(LINK.Vre[4][4]) * LINK.Vim[i][4];
    }
  }
}

/****************************************************************************/
/* void GetEta(int k, Matrix &M, Vector &Eta, struct LOC_GDiag &LINK)

   Purpose: called by GDiag
         Find dispersion and momentum compaction of the map M where
         M is a 4x4 or a 6x6 matrix which has this form

         [ N11  N12  m1  0 ][ N11  N12  N13  N14  m1  0 ]
         [ N21  N22  m2  0 ][ N21  N22  N23  N24  m2  0 ]
         [ 0    0    1   0 ][ N31  N32  N33  N34  m3  0 ]
         [ n1   n2   a   1 ][ N41  N42  N43  N44  m4  0 ]
                            [ 0    0    0    0    1   0 ]
                            [ n1   n2   n3   n4   a   1 ]

         Dispersion:  Eta   = (Inv(I - N)) x m
         Momentum Compaction:Alpha = -(a + n x Eta)


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
static void GetEta(int k, Matrix &M, Vector &Eta, struct LOC_GDiag &LINK)
{
  Vector SmallM;
  Matrix IMinNInv, I;
  int j;

  /* k = 6 in tracy */
  UnitMat(k, I);
  CopyMat(k, I, IMinNInv);
  SubMat(k, M, IMinNInv);
  if (!InvMat(k - 2, IMinNInv))
    printf("(I-N) is singular\n");
  for (j = 0; j <= k - 3; j++)
    SmallM[j] = M[j][k - 2];
  SmallM[4] = 0.0e0;
  SmallM[5] = 0.0e0;
  CopyVec(k, SmallM, Eta);
  LinTrans(k - 2, IMinNInv, Eta);
  /*   Alpha := 0.0d0;
       for j:= 1 TO (k-2) DO
         Alpha := Alpha + M[6,j]*Eta[j];
       Alpha := Alpha + M[6,5]; */
}

/****************************************************************************/
/* void GenB(int k, Matrix &B, Matrix &BInv, Vector &Eta, struct LOC_GDiag &LINK)

   Purpose: called by GDiag

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
void GenB(int k, Matrix &B, Matrix &BInv, Vector &Eta, struct LOC_GDiag &LINK)
{
  int j;

  UnitMat(k, B);
  for (j = 0; j <= k - 3; j++)
    B[j][k - 2] = Eta[j];
  B[5][0] = Eta[1];
  B[5][1] = -Eta[0];
  B[5][2] = Eta[3];
  B[5][3] = -Eta[2];
  CopyMat(k, B, BInv);
  if (!InvMat(k, BInv))
    printf("B is singular\n");
}


/****************************************************************************/
/* void GDiag(int n_, double C, Matrix &A, Matrix &Ainv_, Matrix &R,
           Matrix &M, double *Omega, double *alphac)

   Purpose: called by Ring_MatTwiss, Ring_DATwiss
   
      Input M:     Oneturn transfer matrix

      Output A

                                 [ c  s  0  0 ]
                  -1             [-s  c  0  0 ]
           M  -> A   M A = R =   [ 0  0  c  s ]
                                 [ 0  0 -s  c ]

      The eigenvectors are normalized so that the real and
      imaginary part of vectors 1 and 3 have +1 antisymmetric
      product:

        Vre1 J aivec1 := 1 ; Vre3 J aivec3 := 1 ;

      the eigenvectors 2 and 4 have the opposite normalization.


   Input:
       M oneturn tranfer matrix

   Output:   -1
       A -> A  M A = Rotation

   Return:
       none

   Global variables:
       none

   Specific functions:
       InitJJ, CopyMat, TpMat, UnitMat, GetAinv, MulLMat
       geigen,
       GetEta, GenB
       
   Comments:
       none

****************************************************************************/
void GDiag(int n_, double C, Matrix &A, Matrix &Ainv_, Matrix &R,
           Matrix &M, double &Omega, double &alphac)
{
  struct LOC_GDiag V;
  int j;
  double x1, x2;
  Vector wr, wi, eta;
  Matrix fm, B, Binv;
  int FORLIM;
  double TEMP, TEMP1;

  V.n    = n_;
  V.Ainv = &Ainv_;
  InitJJ(V);
  CopyMat(V.n, M, fm); /* fm<-M */
  TpMat(V.n, fm);  /* fm <- transpose(fm) */
  /* look for eigenvalues and eigenvectors of fm */
  geigen(V.n, fm, V.Vre, V.Vim, wr, wi);
  FORLIM = V.n/2;
  for (j = 1; j <= FORLIM; j++)
  {
    TEMP  = wr[j*2-2]; TEMP1 = wi[j*2-2];
    x1 = sqrt(sqr(TEMP)+sqr(TEMP1));
    TEMP  = wr[j*2-1]; TEMP1 = wi[j*2-1];
    x2 = sqrt(sqr(TEMP)+sqr(TEMP1));
    globval.alpha_rad[j-1] = log(sqrt(x1*x2));
  }

  CopyMat(V.n, M, fm);
  geigen(V.n, fm, globval.Vr, globval.Vi, globval.wr, globval.wi);

  /*  CopyVec(6,wr,globval.wr);
  CopyVec(6,wi,globval.wi);
  CopyMat(6,V.Vre,globval.Vr);
  CopyMat(6,V.Vim,globval.Vi); */

  UnitMat(6, *V.Ainv);
  GetAinv(V);
  CopyMat(6, *V.Ainv, A);
  if (!InvMat(6, A))
    printf("A^-1 script is singular\n");
  if (V.n == 4)
  {
    GetEta(6, M, eta, V);
    GenB(6, B, Binv, eta, V);
    MulLMat(6, B, A);
    MulLMat(6, *V.Ainv, Binv);
    CopyMat(6, Binv, *V.Ainv);
  }
  CopyMat(6, A, R);
  MulLMat(6, M, R);
  MulLMat(6, *V.Ainv, R);
  if (V.n == 4)
  {
    Omega = 0.0;
    alphac = R[5][4] / C;
  }
  if (V.n != 6)
    return;
  if (globval.Cavity_on)
  {
    Omega = GetAngle(R[4][4], R[4][5]) / (2.0 * M_PI);
    alphac = 0.0;
  } else
  {
    Omega = 0.0;
    alphac = R[5][4] / C;
  }
}

/****************************************************************************/
/* Local void eswap(Matrix &t6a, Vector &lamr, Vector &lami, Matrix &r,
                 int i3, int j3)

   Purpose: called by NormEigenVec


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
static void eswap(Matrix &t6a, Vector &lamr, Vector &lami, Matrix &r,
                  int i3, int j3)
{
  double xx;
  int i,i1,i2,j1,j2;

  i2 = 2*i3;
  i1 = i2-1;
  j2 = 2*j3;
  j1 = j2-1;

  for (i = 1; i <= ss_dim/2; i++)
  {
    xx=r[i-1][i3-1];
    r[i-1][i3-1]=r[i-1][j3-1];
    r[i-1][j3-1]=xx;
  }

  for (i = 1; i <= ss_dim; i++)
  {
    xx=t6a[i-1][i1-1];
    t6a[i-1][i1-1]=t6a[i-1][j1-1];
    t6a[i-1][j1-1]=xx;
    xx=t6a[i-1][i2-1];
    t6a[i-1][i2-1]=t6a[i-1][j2-1];
    t6a[i-1][j2-1]=xx;
  }

  xx = lamr[i1-1];
  lamr[i1-1] = lamr[j1-1];
  lamr[j1-1] = xx;

  xx = lamr[i2-1];
  lamr[i2-1] = lamr[j2-1];
  lamr[j2-1] = xx;

  xx = lami[i1-1];
  lami[i1-1] = lami[j1-1];
  lami[j1-1] = xx;

  xx = lami[i2-1];
  lami[i2-1] = lami[j2-1];
  lami[j2-1] = xx;
}

/*****************************************************************************/
/* void NormEigenVec(Matrix &Vr, Matrix &Vi, Vector &wr, Vector &wi, Matrix &t6a)

   Purpose:  called by GetEmittance
       Sort and normalize complex eigenvectors (Vr, Vi)
       with eigenvalues (wr, wi) and store the result in
       t6a. Used for the calculation of generalized sigma matrices
       ala CHAO in emit.

   Input:
       (Vr, Vi) complex eigenvector
       (wr, wi) complex eigen values

   Output:
       t6a matrix of sorted and normalized eigenvectors
       
   Return:
       none

   Global variables:
       none

   Specific functions:
       none

   Comments:
       30/12/02 label 150 removed

****************************************************************************/
void NormEigenVec(Matrix &Vr,Matrix &Vi, Vector &wr, Vector &wi, Matrix &t6a)
{
  Matrix r;
  double rn,sqrn;
  int i,i1,i2,i3,j1,j2,j3;

  for (i = 0; i < ss_dim; i++)
  {
    t6a[i][0] = Vr[i][0];
    t6a[i][1] = Vi[i][0];
    t6a[i][2] = Vr[i][2];
    t6a[i][3] = Vi[i][2];
    t6a[i][4] = Vr[i][4];
    t6a[i][5] = Vi[i][4];
  }

  /* normierung der eigenvektoren */

  for (j1 = 1; j1 <= ss_dim; j1 += 2)
  {
    j2 = j1 + 1;
    j3 = j2 / 2;
    rn = 0e0;
    for (i1 = 1; i1 <= ss_dim; i1+= 2)
    {
      i2 = i1 + 1;
      i3 = i2 / 2;
      r[i3-1][j3-1] = t6a[i1-1][j1-1]*t6a[i2-1][j2-1]-t6a[i2-1][j1-1]*t6a[i1-1][j2-1];
      rn += r[i3-1][j3-1];
    }

    for (i = 1; i <= ss_dim/3; i++)
    {
      r[i-1][j3-1] = fabs(r[i-1][j3-1]/rn);
    }

    if (rn < 0)
    {
      for (i = 1; i <= ss_dim; i++)
      {
        t6a[i-1][j2-1] = -t6a[i-1][j2-1];
      }
    }
 
    sqrn = sqrt(fabs(rn)); /* take the norm of rn */
 
    for (i = 1; i <= ss_dim; i++)
    {
      t6a[i-1][j1-1]=t6a[i-1][j1-1]/sqrn;
      t6a[i-1][j2-1]=t6a[i-1][j2-1]/sqrn;
    }

  }

  if ((r[0][0] > r[1][0]) && (r[0][0] > r[2][0])) goto L140;
  if ((r[0][1] > r[1][1]) && (r[0][1] > r[2][1])) goto L130;
  eswap(t6a,wr,wi,r,1,3);
  goto L140;

L130:
  eswap(t6a,wr,wi,r,1,2);

L140:
//  if ((r[1][1] > r[0][1]) && (r[1][1] > r[2][1])) goto L150;
  if ((r[1][1] > r[0][1]) && (r[1][1] > r[2][1]))  return;
  eswap(t6a,wr,wi,r,2,3);

//L150:
//
}
