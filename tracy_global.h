#ifndef TRACY_GLOBAL_H
#define TRACY_GLOBAL_H


#define PLANES 2
#define BPM_MAX 120

typedef struct globvalrec {
  double        dPcommon,       // dp for numerical differentiation 
                dPparticle;     // energy deviation 
  double        delta_RF;       // RF acceptance 
  Vector2       TotalTune;      // transverse tunes 
  double        Omega,
                U0,             // energy lost per turn in keV 
                Alphac;         // alphap 
  Vector2       Chrom;          // chromaticities 
  double        Energy;         // ring energy 
  long          Cell_nLoc,      // number of elements in a cell
                Elem_nFam,      // number of families 
                CODimax;        /* maximum number of cod search before
				   failing */
  double        CODeps;         // precision for closed orbit finder
  Vector        CODvect;        // closed orbit; beam position at the first element of lattice 
  
  // family index for special elements 
  long int       bpm;          // family index of bpm
  long int       hcorr;          // family index of horizontal corrector which are used for orbit correction
  long int       vcorr;          // family index of vertical corrector which are used for orbit correction
  long int       qt;           // family index of skew quadrupole
  int            gs;             // family number of start girder  
  int            ge;             // family number of end girder 
  
  // matrix
  Matrix        OneTurnMat,     // oneturn matrix 
                Ascr,
                Ascrinv,
                Vr,             // real part of the eigenvectors 
                Vi;             // imaginal par of the eigenvectors 

  bool          MatMeth,        // matrix method 
                Cavity_on,      // if true, cavity turned on 
                radiation,      // if true, radiation turned on 
                emittance,
                quad_fringe,    /* dipole- and quadrupole hard-edge
				   fringe fields. */
                H_exact,        // "small ring" Hamiltonian. 
                pathlength,     // absolute path length
                stable,
                Aperture_on,
                EPU,
                wake_on;

  double        dE,             // energy loss
                alpha_rad[DOF], // damping coeffs.
                D_rad[DOF],     // diffusion coeffs (Floquet space)
                J[DOF],         // partition numbers
                tau[DOF];       // damping times
  bool          IBS;            // intrabeam scattering
  double        Qb,             // bunch charge
                D_IBS[DOF];     // diffusion matrix (Floquet space)
  Vector        wr, wi;         // real and imaginary part of eigenvalues
  Vector3       eps,            // 3 motion invariants
                epsp;           /* transverse and longitudinal projected
				   emittances */
  int           RingType;       // 1 if a ring (0 if transfer line)

  int           nr_cpus;        // used by parallelized functions
  
 long bpm_list[BPM_MAX];        /* list of position for bpms into the lattice */
 long hcorr_list[BPM_MAX];      /* list of position for horizontal correctors */
 long vcorr_list[BPM_MAX];      /* list of position for vertical correctors  */
 long qt_list[BPM_MAX];        /* list of position for vertical correctors into the lattice */


} globvalrec;


struct DriftType {
  Matrix D55; // Linear matrix
};


struct MpoleType {
  int         Pmethod;   // Integration Method
  int         PN;        // Number of integration steps
  long        quadFF1;         /* Entrance quadrupole Fringe field flag */
  long        quadFF2;         /* Exit quadrupole Fringe field flag */
  double      quadFFscaling;         /* quadrupole Fringe field scaling factor flag */
  long        sextFF1;         /* Entrance sextupole Fringe field flag */
  long        sextFF2;         /* Exit sextupole Fringe field flag */
  
  bool         Status;         /* specific for correctors used for orbit correction. If true, use the corrector 
                                  to correct orbit, if false, not use. */
  
  // Displacement Errors
  Vector2     PdSsys;    // systematic displacement error[m] 
  Vector2     PdSrms;    // rms value of the displacement error[m]
  Vector2     PdSrnd;    // (normal)random scale factor to displacement error PdSrms
  // Roll angle
  double      PdTpar;    // design rotation angle, if not equal zero, then skew multipole[deg]
  double      PdTsys;    // systematic [deg]
  double      PdTrms;    // rms rotation error of the element[deg]
  double      PdTrnd;    // (normal)random scale factor to rotation error PdTrms
  // Multipole strengths
  mpolArray   PBpar;     // design field strength
  mpolArray   PBsys;     // systematic multipole errors
  mpolArray   PBrms;     // rms multipole field errors
  mpolArray   PBrnd;     // random scale factor of rms error PBrms
  mpolArray   PB;        // total field strength(design,sys,rms)
  int         Porder;    // The highest order in PB
  int         n_design;  // multipole order (design)
  pthicktype  Pthick;    // thick element
  // Bending Angles
  double PTx1;           // horizontal entrance angle [deg]
  double PTx2;           // horizontal exit angle [deg]
  double Pgap;           // total magnet gap [m] 
  double Pirho;          // angle/strength of the dipole, 1/rho [1/m] 
  double Pc0, Pc1, Ps1;  // corrections for roll error of bend 
  Matrix AU55,           // Upstream 5x5 matrix 
    AD55;           // Downstream 5x5 matrix 
};

const int  n_harm_max = 10;

struct WigglerType {
  int Pmethod;                // Integration Method 
  int PN;                     // number of integration steps 
  // Displacement Error 
  Vector2 PdSsys;             // systematic [m]  
  Vector2 PdSrms;             // rms [m] 
  Vector2 PdSrnd;             // random number 
  // Roll angle 
  double PdTpar;              // design [deg] 
  double PdTsys;              // systematic [deg] 
  double PdTrms;              // rms [deg] 
  double PdTrnd;              // random number 
  double lambda;              // lambda 
  int    n_harm;              // no of harmonics
  int    harm[n_harm_max];    // harmonic number
  double BoBrhoV[n_harm_max]; // B/Brho vertical
  double BoBrhoH[n_harm_max]; // B/Brho horizontal 
  double kxV[n_harm_max];     // vertical kx 
  double kxH[n_harm_max];     // horizontal kx 
  double phi[n_harm_max];     // phi 
  mpolArray PBW;
  Matrix W55;                 // Transport matrix 
  int Porder;                 // The highest order in PB 
};


const int  i_max_FM = 100, j_max_FM = 100, k_max_FM = 1000;

struct FieldMapType {
  int     n_step; // number of integration steps
  int     n[3];                                         // no of steps
  double  scl;
  double  xyz[3][i_max_FM][j_max_FM][k_max_FM];         // [x, y, z]
  double  B[3][i_max_FM][j_max_FM][k_max_FM];           // [B_x, B_y, B_z]
  double  **AxoBrho, **AxoBrho2, **AyoBrho, **AyoBrho2; // Ax(y, s), Ay(x, s)
};


/* ID Laurent */
#define IDXMAX 500
#define IDZMAX 100

struct InsertionType {
  int Pmethod;      /* Integration Method */
  int PN;           /* number of integration steps */
  char fname1[100]; /* Filename for insertion description: first order */
  char fname2[100]; /* Filename for insertion description: second order */
  int nx1;           /* Horizontal point number */
  int nx2;           /* Horizontal point number */
  int nz1;           /* Vertical point number */
  int nz2;           /* Vertical point number */
  double scaling1;   /* static scaling factor as in BETA ESRF first order*/
  double scaling2;   /* static scaling factor as in BETA ESRF second order*/
  bool linear;      /* if true linear interpolation else spline */
  bool firstorder;  /* true if first order kick map loaded */
  bool secondorder; /* true if second order kick map loaded */
  double tabx1[IDXMAX]; /* spacing in H-plane */
  double tabz1[IDZMAX]; /* spacing in V-plane */
  double tabx2[IDXMAX]; /* spacing in H-plane */
  double tabz2[IDZMAX]; /* spacing in V-plane */
  double thetax2[IDZMAX][IDXMAX], thetax1[IDZMAX][IDXMAX]; /* 1 for 1st order */
  double thetaz2[IDZMAX][IDXMAX], thetaz1[IDZMAX][IDXMAX];
  double **tx2, **tz2, **f2x2, **f2z2; // used for splie2 and splin2 (Spline interpolation)
  double **tx1, **tz1, **f2x1, **f2z1; // used for splie2 and splin2
  double *TabxOrd1, *TabzOrd1; // tab of x and z meshes from Radia code in increasing order
  double *TabxOrd2, *TabzOrd2; // tab of x and z meshes from Radia code in increasing order
  
  /* Displacement Error */
  Vector2 PdSsys;   /* systematic [m]  */
  Vector2 PdSrms;   /* rms [m] */
  Vector2 PdSrnd;   /* random number */
  /* Roll angle */
  double PdTpar;    /* design [deg] */
  double PdTsys;    /* systematic [deg] */
  double PdTrms;    /* rms [deg] */
  double PdTrnd;    /* random number */
  Matrix K55;        /* Transport matrix:kick part */
  Matrix D55;        /* Transport matrix:drift part */
  Matrix KD55;       /* Transport matrix:concatenation of kicks and drifts */
  int Porder;        /* The highest order in PB */
};

struct CavityType {
  double Pvolt;   // Vrf [V]
  double Pfreq;   // Vrf [Hz]
  double phi;     // RF phase
  int    Ph;      // Harmonic number
};

struct CellType;

const int  Spreader_max = 10;

struct SpreaderType {
  double    E_max[Spreader_max];      // energy levels in increasing order
  CellType  *Cell_ptrs[Spreader_max];
};

struct RecombinerType {
  double    E_min;
  double    E_max;
};

struct SolenoidType {
  int         N;         // Number of integration steps
  // Displacement Errors
  Vector2     PdSsys;    // systematic [m] 
  Vector2     PdSrms;    // rms [m]
  Vector2     PdSrnd;    // random number
  // Roll angle
  double      dTpar;     // design [deg]
  double      dTsys;     // systematic [deg]
  double      dTrms;     // rms [deg]
  double      dTrnd;     // random number
  double      BoBrho;    // normalized field strength
};

struct elemtype {
  partsName PName;   /* Element name */
  double PL;         /* Length[m]    */
  PartsKind Pkind;   /* Enumeration  for magnet types */
  union
  {
    DriftType      *D;   // Drift
    MpoleType      *M;   // Multipole
    WigglerType    *W;   // Wiggler
    FieldMapType   *FM;  // Field Map
    InsertionType  *ID;  // Insertion
    CavityType     *C;   // Cavity
    SpreaderType   *Spr; // Spreader
    RecombinerType *Rec; // Recombiner
    SolenoidType   *Sol; // Solenoid
  };
};

struct ElemFamType {
  elemtype    ElemF;    /* Structure (name, type) */
  int         nKid;         /* number of Kids in a family */
  int         KidList[nKidMax]; /* list of kid index in the total lattice*/
  int         NoDBN;
  DBNameType  DBNlist[nKidMax];
};

// LEGO block structure for each element of the lattice
struct CellType {
  long int              Fnum;        // Element Family #
  long int              Knum;        // Element Kid #
  double           S;           // Position in the ring [m]
  CellType*        next_ptr;    // pointer to next cell (for tracking)
  Vector2          dS,          // displacement error of the element[m]
                   dT;          // rotation error of the element[degree], dT = (cos(dT), sin(dT))
  elemtype         Elem;        // Structure (name, type)
  Vector2          Nu,          // Phase advances 
                   Alpha,       // Alpha functions (redundant)
                   Beta,        // beta fonctions (redundant)
                   Eta, Etap;   // dispersion and its derivative (redundant)
  Vector           BeamPos;     // position of the beam at the cell; 
  Matrix           A,           // Floquet space to phase space transformation
                   sigma;       // sigma matrix (redundant)
  Vector2          maxampl[PLANES]; /* Horizontal and vertical physical
				       apertures:
				         maxampl[X_][0] < x < maxampl[X_][1]
					 maxampl[Y_][0] < y < maxampl[Y_][1] */
};

#endif
