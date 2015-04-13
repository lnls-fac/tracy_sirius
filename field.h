 /* Author:	 Johan Bengtsson

   Definitions:  Polymorphic number class.              */

#ifndef FIELD_H
#define FIELD_H

const int  max_str = 132;

const int  n_m2    = 21;  // no of 2nd moments

#if ORDER == 1
const int  ss_dim  = 6;   // state space dimension
#else
const int  ss_dim  = 6+1; /* state space dimension:
			     phase space and parameter dependence */
#endif

// spatial components
enum spatial_index { X_ = 0, Y_ = 1, Z_ = 2 };

// phase space components
// (Note, e.g. spin components should be added here)
enum ps_index { x_ = 0, px_ = 1, y_ = 2, py_ = 3, delta_ = 4, ct_ = 5 };

#if ORDER == 1
typedef double  tps_buf[ss_dim+1]; // const. and linear terms
#endif

template<typename T> class ss_vect;

typedef int  iVector[ss_dim];

// Polymorphic class for floating point and TPSA

struct MNF_struct;

class tps {
 public:
  tps(void);
  tps(const double);
  tps(const double, const int);
  tps(const tps &);
  ~tps(void);

  // initialize TPSA library
   friend void TPSAEps(const double);
  // trace level for TPSALib and LieLib
  friend void idprset(const int);

  const double cst(void) const;
  double operator[](const int) const;
  double operator[](const int []) const;
  void pook(const int [], const double);

  tps& operator=(const double);
  tps& operator+=(const double);
  tps& operator-=(const double);
  tps& operator*=(const double);
  tps& operator/=(const double);

  tps& operator=(const tps &);
  tps& operator+=(const tps &);
  tps& operator-=(const tps &);
  tps& operator*=(const tps &);
  tps& operator/=(const tps &);

  friend double getmat(const ss_vect<tps> &map, const int i, const int j);
  friend void putmat(ss_vect<tps> &map, const int i, const int j,
                     const double r);
#if ORDER == 1
  friend void dacct_(const ss_vect<tps> &x, const int i,
                     const ss_vect<tps> &y, const int j,
                     ss_vect<tps> &z, const int k);
#endif

  friend istream& operator>>(istream &, tps &);
  friend ostream& operator<<(ostream &, const tps &);

  friend double abs(const tps &);
  friend double abs2(const tps &);
  friend tps sqrt(const tps &);
  friend tps sqr(const tps &);
  friend tps pow(const tps &, const int);
  friend tps exp(const tps &);
  friend tps log(const tps &);
  friend tps sin(const tps &);
  friend tps cos(const tps &);
  friend tps tan(const tps &);
  friend tps asin(const tps &);
  friend tps acos(const tps &);
  friend tps atan(const tps &);
  friend tps sinh(const tps &);
  friend tps cosh(const tps &);

  friend tps Der(const tps &, const int);
  friend tps LieExp(const tps &, const tps &);
  friend tps PB(const tps &, const tps &);
  friend tps Take(const tps &, const int);

  // R(nd2, nv) = P(nd2, nd2)*Q(nd2, nv)
  friend ss_vect<tps> operator*(const ss_vect<tps> &, const ss_vect<tps> &);
  // R(nv, nv) = P(nv, nv)*Q(nv, nv)
  friend void CCT(const tps x[], const int n_x, const tps y[], const int n_y,
		  tps z[], const int n_z);
  friend tps operator*(const tps &, const ss_vect<tps> &);

  friend ss_vect<tps> FExpo(const tps &, const ss_vect<tps> &,
			    const int, const int, const int);
  friend ss_vect<tps> LieExp(const tps &, const ss_vect<tps> &);
  // Q(nv, nv) = P(nd2, nd2)^-1
  friend ss_vect<tps> Inv(const ss_vect<tps> &);
  // Q(nv, nv) = P(nv, nv)^-1
  friend ss_vect<tps> Inv_Ext(const ss_vect<tps> &);
  friend ss_vect<tps> PInv(const ss_vect<tps> &, const iVector &);
  friend void GoFix(const ss_vect<tps> &, ss_vect<tps> &,
		    ss_vect<tps> &, const int);
  friend MNF_struct MapNorm(const ss_vect<tps> &, const int);
  friend ss_vect<tps> MapNormF(const ss_vect<tps> &, ss_vect<tps> &,
			       ss_vect<tps> &, ss_vect<tps> &,
			       ss_vect<tps> &, const int, const int);
  friend ss_vect<tps> dHdJ(const tps &);
  friend void CtoR(const tps &, tps &, tps &);
  friend tps RtoC(const tps &, const tps &);
  friend tps LieFact_DF(const ss_vect<tps> &, ss_vect<tps> &);
  friend ss_vect<tps> FlowFact(const ss_vect<tps> &);
  friend tps Intd(const ss_vect<tps> &, const double);
  friend ss_vect<tps> Difd(const tps &, const double);
  friend ss_vect<tps> Taked(const ss_vect<tps> &, const int);
 private:
#if ORDER == 1
  tps_buf  ltps;  // linear TPSA
#else
  int     intptr; // index used by Fortran implementation
#endif
  double  r;      // floating-point calc. if intptr = 0
};


// Class for single particle phase space dynamics

// pre-declare template friend functions: f<>()
template<typename T>
ss_vect<T> operator+(const ss_vect<T> &);
template<typename T>
ss_vect<T> operator-(const ss_vect<T> &);

template<typename T> class ss_vect {
 public:
  typedef T value_type;

  ss_vect(void);
// Let's the compiler synthetize the copy constructor
//  ss_vect(const T &a) { }
//  ss_vect(const ss_vect<T> &a) { }
  template<typename U>
    ss_vect(const U &);
  template<typename U>
    ss_vect(const ss_vect<U> &);

  ss_vect<double> cst(void) const;
  T& operator[](const int i) { return ss[i]; }
  const T& operator[](const int i) const { return ss[i]; }

  ss_vect<T>& operator*=(const double);
  ss_vect<T>& operator*=(const tps &);

  ss_vect<T>& operator=(const ss_vect<T> &);
  ss_vect<T>& operator+=(const ss_vect<T> &);
  ss_vect<T>& operator-=(const ss_vect<T> &);

  friend ss_vect<T> operator+<>(const ss_vect<T> &);
  friend ss_vect<T> operator-<>(const ss_vect<T> &);

  friend ss_vect<double> operator+(const ss_vect<double> &,
				   const ss_vect<double> &);
  friend ss_vect<tps> operator+(const ss_vect<tps> &, const ss_vect<double> &);
  friend ss_vect<tps> operator+(const ss_vect<double> &, const ss_vect<tps> &);
  friend ss_vect<tps> operator+(const ss_vect<tps> &, const ss_vect<tps> &);

  friend ss_vect<double> operator-(const ss_vect<double> &,
				   const ss_vect<double> &);
  friend ss_vect<tps> operator-(const ss_vect<tps> &, const ss_vect<double> &);
  friend ss_vect<tps> operator-(const ss_vect<double> &, const ss_vect<tps> &);
  friend ss_vect<tps> operator-(const ss_vect<tps> &, const ss_vect<tps> &);

//  friend ss_vect<double> operator*(const ss_vect<tps> &,
//				   const ss_vect<double> &);
  // R(nd2, nv) = P(nd2, nd2)*Q(nd2, nv)
  friend ss_vect<tps> operator*(const ss_vect<tps> &, const ss_vect<tps> &);
  // R(nv, nv) = P(nv, nv)*Q(nv, nv)
  friend ss_vect<tps> CCT(const ss_vect<tps> &, const ss_vect<tps> &);
  friend tps operator*(const tps &, const ss_vect<tps> &);

  template<typename CharT, class Traits>
    friend basic_istream<CharT, Traits>&
    operator>>(basic_istream<CharT, Traits> &, ss_vect<tps> &);

  template<typename CharT, class Traits>
    friend basic_ostream<CharT, Traits>&
    operator<<(basic_ostream<CharT, Traits> &, const ss_vect<T> &);

  ss_vect<T> zero(void);
  ss_vect<tps> identity(void);

  friend ss_vect<tps> FExpo(const tps &, const ss_vect<tps> &,
			    const int, const int, const int);
  friend ss_vect<tps> LieExp(const tps &, const ss_vect<tps> &);
  // Q(nv, nv) = P(nd2, nd2)^-1
  friend ss_vect<tps> Inv(const ss_vect<tps> &);
  // Q(nv, nv) = P(nv, nv)^-1
  friend ss_vect<tps> Inv_Ext(const ss_vect<tps> &);
  friend ss_vect<tps> PInv(const ss_vect<tps> &, const iVector &);
  friend void GoFix(const ss_vect<tps> &, ss_vect<tps> &,
		    ss_vect<tps> &, const int);
  friend MNF_struct MapNorm(const ss_vect<tps> &, const int);
  friend ss_vect<tps> MapNormF(const ss_vect<tps> &, ss_vect<tps> &,
			       ss_vect<tps> &, ss_vect<tps> &,
			       ss_vect<tps> &, const int, const int);
  friend void dHdJ(const tps &, ss_vect<tps> &);
  friend void CtoR(const tps &, tps &, tps &);
  friend tps RtoC(const tps &, const tps &);
  friend tps LieFact_DF(const ss_vect<tps> &, ss_vect<tps> &);
  friend tps LieFact(const ss_vect<tps> &);
  friend ss_vect<tps> FlowFact(const ss_vect<tps> &);
  friend tps Intd(const ss_vect<tps> &, const double);
  friend ss_vect<tps> Difd(const tps &, const double);
  friend ss_vect<tps> Taked(const ss_vect<tps> &, const int);
 private:
  // (Note, e.g. spin components should be added here)
  T  ss[ss_dim];
};


typedef ss_vect<double>  Vector;
typedef Vector           Matrix[ss_dim];


typedef struct MNF_struct
{
  tps           K;       // new effective Hamiltonian
  ss_vect<tps>  A0;      // transformation to fixed point
  ss_vect<tps>  A1;      // (linear) transformation to Floquet space
  tps           g;       /* generator for nonlinear transformation to
			    Floquet space */
  ss_vect<tps>  map_res; // residual map
} MNF_struct;


// partial template-class specialization
// primary version: is_double<>
template<typename T>
class is_double { };

// partial specialization
template<>
class is_double<double> {
 public:
  static inline double cst(const double x) { return x; }
};

// partial specialization
template<>
class is_double<tps> {
 public:
  static inline double cst(const tps &x) { return x.cst(); }
};


template<typename T>
inline T sqr(const T &a)
{ return a*a; }

//overloading operator functions, specific for 
//the user-defined data type tps
inline tps operator+(const tps &a, const tps &b)
{ return tps(a) += b; }

inline tps operator+(const tps &a, const double b)
{ return tps(a) += b; }

inline tps operator+(const double a, const tps &b)
{ return tps(a) += b; }

inline tps operator-(const tps &a, const tps &b)
{ return tps(a) -= b; }

inline tps operator-(const tps &a, const double b)
{ return tps(a) -= b; }

inline tps operator-(const double a, const tps &b)
{ return tps(a) -= b; }

inline tps operator*(const tps &a, const tps &b)
{ return tps(a) *= b; }

inline tps operator*(const tps &a, const double b)
{ return tps(a) *= b; }

inline tps operator*(const double a, const tps &b)
{ return tps(a) *= b; }

inline tps operator/(const tps &a, const tps &b)
{ return tps(a) /= b; }

inline tps operator/(const tps &a, const double b)
{ return tps(a) /= b; }

inline tps operator/(const double a, const tps &b)
{ return tps(a) /= b; }


inline tps operator+(const tps &x)
{ return tps(x); }

inline tps operator-(const tps &x)
{ return tps(x) *= -1.0; }


inline bool operator>(const tps &a, const tps &b)
{ return a.cst() > b.cst(); }

inline bool operator>(const tps &a, const double b)
{ return a.cst() > b; }

inline bool operator>(const double a, const tps &b)
{ return a > b.cst(); }


inline bool operator<(const tps &a, const tps &b)
{ return a.cst() < b.cst(); }

inline bool operator<(const tps &a, const double b)
{ return a.cst() < b; }

inline bool operator<(const double a, const tps &b)
{ return a < b.cst(); }


inline bool operator>=(const tps &a, const tps &b)
{ return a.cst() >= b.cst(); }

inline bool operator>=(const tps &a, const double b)
{ return a.cst() >= b; }

inline bool operator>=(const double a, const tps &b)
{ return a >= b.cst(); }


inline bool operator<=(const tps &a, const tps &b)
{ return a.cst() <= b.cst(); }

inline bool operator<=(const tps &a, const double b)
{ return a.cst() <= b; }

inline bool operator<=(const double a, const tps &b)
{ return a <= b.cst(); }


inline bool operator==(const tps &a, const tps &b)
{ return a.cst() == b.cst(); }

inline bool operator==(const tps &a, const double b)
{ return a.cst() == b; }

inline bool operator==(const double a, const tps &b)
{ return a == b.cst(); }


inline bool operator!=(const tps &a, const tps &b)
{ return a.cst() != b.cst(); }

inline bool operator!=(const tps &a, const double b)
{ return a.cst() != b; }

inline bool operator!=(const double a, const tps &b)
{ return a != b.cst(); }

/******************************************************
template<typename T>
inline ss_vect<T>::ss_vect(void)

  Purpose:
     Initialize 6-D vector(T:double) or 6*6 matrix(T:tps) ss, based on
     the type of T, if ORDER = 1
      Initialize 7-D vector or 7*7 matrix ss, based on
     the type of T, if ORDER != 1
     
    
     
  
  
  Parameters:
      ss_dim:  6      if ORDER = 1
      ss_dim:  6+1    if ORDER != 1  
     
******************************************************/

template<typename T>
inline ss_vect<T>::ss_vect(void)
{
  int  i;

  for (i = 0; i < ss_dim; i++)
    ss[i] = T();
}

template<typename T>
template<typename U>
inline ss_vect<T>::ss_vect(const U &a) : ss(a) { }

template<typename T>
template<typename U>
inline ss_vect<T>::ss_vect(const ss_vect<U> &a)
{
  int              i;

  for (i = 0; i < ss_dim; i++)
    ss[i] = a[i];
 }

#endif
