/* Tracy-2

   J. Bengtsson, CBP, LBL      1990 - 1994   Pascal version
                 SLS, PSI      1995 - 1997
   M. Boege      SLS, PSI      1998          C translation
   L. Nadolski   SOLEIL        2002          Link to NAFF, Radia field maps
   J. Bengtsson  NSLS-II, BNL  2004 -        

*/

// macros

#define sqr(x)   ((x)*(x))
#define cube(x)  ((x)*(x)*(x))

#define fract(x) ((x)-(int)(x))
#define nint(x) ((x) < 0 ? ((long)(x-0.5)) : ((long)(x+0.5))) 

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define sgn(n) ((n > 0) ? 1 : ((n < 0) ? -1 : 0)) 


extern long rseed0;
extern long rseed;
extern double normcut_;

void iniranf(const long i);
void newseed(void);
double ranf(void);
void setrancut(const double cut);
double normranf(void);

double dtor(const double d);
double GetAngle(const double x, const double y);

void CopyVec(const int n, const Vector &a, Vector &b);
void AddVec(const int n, const Vector &a, Vector &b);
void SubVec(const int n, const Vector &a, Vector &b);
double xabs(long n, Vector &x);

void UnitMat(const int n, Matrix &a);
void ZeroMat(const int n, Matrix &a);
void CopyMat(const int n, const Matrix &a, Matrix &b);
void AddMat(const int n, const Matrix &a, Matrix &b);
void SubMat(const int n, const Matrix &a, Matrix &b);
void LinTrans(const int n, const Matrix &a, Vector &x);
void MulLMat(const int n, const Matrix &a, Matrix &b);
void MulRMat(const int n, Matrix &a, const Matrix &b);
double TrMat(const int n, const Matrix &a);
void TpMat(const int n, Matrix &a);
double DetMat(const int n, const Matrix &a);
bool InvMat(const int n, Matrix &a);
void prtmat(const int n, const Matrix &a);
void MulcMat(const int n, const double c, Matrix &A);
