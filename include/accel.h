#include "presto.h"
#include "accelsearch_cmd.h"

/* #define ACCEL_USELEN 64000 */
#define ACCEL_USELEN 32000
#define ACCEL_NUMBETWEEN 2
/* Stepsize in Fourier Freq */
#define ACCEL_DR  0.5
/* Reciprocal of ACCEL_DR */
#define ACCEL_RDR 2
/* Stepsize in Fourier F-dot */
#define ACCEL_DZ  2
/* Reciprocal of ACCEL_DZ */
#define ACCEL_RDZ 0.5

typedef struct accelobs{
  long long N;         /* Number of data points in observation */
  long long numbins;   /* Number of spectral bins in the file */
  long long lobin;     /* Lowest spectral bin present in the file */
  long long highestbin;/* Highest spectral bin present in the file */
  int fftlen;          /* Length of short FFTs to us in search */
  int numharm;         /* Number of harmonics searched */
  int numz;            /* Number of f-dots searched */
  int numbetween;      /* Highest fourier freq resolution (2=interbin) */
  int numzap;          /* Number of birdies to zap */
  double dt;           /* Data sample length (s) */           
  double T;            /* Total observation length */
  double rlo;          /* Minimum fourier freq to search */
  double rhi;          /* Maximum fourier freq to search */
  double dr;           /* Stepsize in fourier freq (1/numbetween) */
  double zlo;          /* Minimum fourier fdot to search */
  double zhi;          /* Maximum fourier fdot to search */
  double dz;           /* Stepsize in fourier fdot */
  double baryv;        /* Average barycentric velocity during observation */
  float nph;           /* Freq 0 level if present (1 otherwise) */
  float sigma;         /* Cutoff sigma to choose a candidate */
  float *powcut;       /* Cutoff powers to choose a cand (per harmsummed) */
  double *lobins;      /* The low Fourier freq boundaries to zap (RFI) */
  double *hibins;      /* The high Fourier freq boundaries to zap (RFI) */
  long long *numindep; /* Number of independent spectra (per harmsummed) */
  FILE *fftfile;       /* The FFT file that we are analyzing */
  FILE *workfile;      /* A text file with candidates as they are found */
} accelobs;

typedef struct accelcand{
  float power;         /* Power level (normalized) */
  float sigma;         /* Equivalent sigma based on numindep (above) */
  int numharm;         /* Number of harmonics summed */
  double r;            /* Fourier freq */
  double z;            /* Fourier f-dot */
} accelcand;

typedef struct kernel{
  int z;               /* The fourier f-dot of the kernel */
  int fftlen;          /* Number of complex points in the kernel */
  int numgoodbins;     /* The number of good points you can get back */
  int numbetween;      /* Fourier freq resolution (2=interbin) */
  int kern_half_width; /* Half width (bins) of the raw kernel. */
  fcomplex *data;      /* The FFTd kernel itself */
} kernel;

typedef struct subharminfo{
  int numharm;       /* The number of sub-harmonics */
  int harmnum;       /* The sub-harmonic number (fundamental = numharm) */
  int zmax;          /* The maximum Fourier f-dot for this harmonic */
  int numkern;       /* Number of kernels in the vector */
  kernel *kern;      /* The kernels themselves */
} subharminfo;

typedef struct ffdotpows{
  int numrs;          /* Number of Fourier freqs present */
  int numzs;          /* Number of Fourier f-dots present */
  int rlo;            /* Lowest Fourier freq present */
  int zlo;            /* Lowest Fourier f-dot present */
  float **powers;     /* Matrix of the powers */
} ffdotpows;

subharminfo *create_subharminfo_vect(int numharm, int zmax);
void free_subharminfo_vect(int numharm, subharminfo *shi);
ffdotpows *subharm_ffdot_plane(int numharm, int harmnum, 
			       double fullrlo, double fullrhi, 
			       subharminfo *shi, accelobs *obs);
ffdotpows *copy_ffdotpows(ffdotpows *orig);
void free_ffdotpows(ffdotpows *ffd);
void add_ffdotpows(ffdotpows *fundamental, ffdotpows *subharmonic, 
		   int numharm, int harmnum);
void search_ffdotpows(ffdotpows *ffdot, int numharm, 
		      accelobs *obs, GSList *cands);
void create_accelobs(FILE *infile, FILE *workfile, 
		     accelobs *obs, infodata *idata, Cmdline *cmd);
void free_accelobs(accelobs *obs);
accelcand *create_accelcand(float power, float sigma, 
			    int numharm, double r, double z);
int compare_accelcand(gconstpointer ca, gconstpointer cb);
void print_accelcand(gpointer data, gpointer user_data);
void free_accelcand(gpointer data, gpointer user_data);
