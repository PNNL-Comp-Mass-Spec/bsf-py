#include <Python.h>
#include <numpy/arrayobject.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "bsf-core/BSFCoreDll.h"
#include <time.h>
  #include <fstream>
#include <sstream>

using namespace std;

struct BSFResult
{
  time_t time;
  unsigned max;
  unsigned min;
  unsigned* histo;
  uint64_t total;
  uint64_t nonzeros;
  double nonzero_percent;
  int msec;
};

static PyObject* test(PyObject *self, PyObject *args)
{
  char* str;
  int len;
  if (!PyArg_ParseTuple(args, "s", &str))
    return NULL;
  len = strlen(str);
  return Py_BuildValue("i", len);
}

template<typename T>
T **ptrvector(long n) {
   T **v;
   v=(T **)malloc( (n*sizeof(T)));
   if (!v)   {
      printf("In **ptrvector. Allocation of memory for 2d array failed.");
      exit(0);
   }
   return v;
}

void free_Carrayptrs(unsigned **v)  {
  free((char*) v);
}

void free_Carrayptrs(uint64_t **v)  {
  free((char*) v);
}

template<typename T>
T **py2D_to_Carrayptrs(PyArrayObject *arrayin) {
   T **c, *a;
   int i,n,m;

   n=arrayin->dimensions[0];
   m=arrayin->dimensions[1];
   c=ptrvector<T>(n);
   a=(T *) arrayin->data; /* pointer to arrayin data as uint64 */
   for ( i=0; i<n; i++) {
      c[i]=a+i*m;
   }
   return c;
}

template<typename T>
T **py2D_to_Carrayptrs(PyArrayObject *arrayin, int n, int m) {
   T **c, *a;
   int i;

   c=ptrvector<T>(n);
   a=(T *) arrayin->data; /* pointer to arrayin data as uint64 */
   for ( i=0; i<n; i++) {
      c[i]=a+i*m;
   }
   return c;
}

/* ==== Check that PyArrayObject is a uint64 type and a 2d array ==============
    return 1 if an error and raise exception */
int not_2Duint64(PyArrayObject *mat) 
{
  if (mat->descr->type_num != NPY_UINT64 || mat->nd != 2) 
  {
    PyErr_SetString(PyExc_ValueError, "In not_2Duint64: array must be of type uint64 and 2 dimensional (n x m).");
    return 1;
  }
  return 0;
}

static PyObject* func(PyObject* self, PyObject* args) {
  PyObject *list2_obj;
  if (!PyArg_ParseTuple(args, "O", &list2_obj))
    return NULL;

  double **list2;
  
  //Create C arrays from numpy objects:
  int typenum = NPY_DOUBLE;
  PyArray_Descr *descr;
  descr = PyArray_DescrFromType(typenum);
  npy_intp dims[3];
  if (PyArray_AsCArray(&list2_obj, (void **)&list2, dims, 2, descr) < 0) {
    PyErr_SetString(PyExc_TypeError, "error converting to c array");
    return NULL;
  }
  // printf("2D: %f, 3D: %f.\n", list2[3][1]);
  return Py_BuildValue("d", list2[0][0]);
}

#ifdef DEBUG
  // Write 2D binary array to binary file
  inline void write_binary(uint64_t** _uint64, int nrow, int ncol, const char* filename) {
    std::ofstream bin_file(filename, std::ios::binary);
    
    printf("Writing:\t%s\n", filename);
    
    bin_file.write((char*) &nrow, sizeof(int));
    bin_file.write((char*) &ncol, sizeof(int));
    for(int i = 0; i < ncol; i++) {
      for(int j = 0; j < nrow; j++) {
        bin_file.write((char*) &_uint64[i][j], sizeof(uint64_t));
      }
    }
    bin_file.close();
  }

  inline char* decimal_to_binary(uint64_t num) {
      char* bitset = new char[64];
      for(uint64_t i=0; i<64; ++i) {
          if((num & (1ull << i)) != 0) {
              bitset[63-i] = '1';
          } else {
              bitset[63-i] = '0';
          }
      }
      for(uint64_t i=0; i<64; ++i) {
          printf("%c", bitset[63-i]);
      }
      printf("\n");
      return bitset;
  }

  inline void writeMatrix(const uint64_t** _uint64, int ncol, int nrow) {
    std::ofstream outfile;
    outfile.open("debug_matrix.txt");
    unsigned i, j;
    for (i = 0; i < nrow; i++){
      for(j = 0; j < ncol; j++){
        outfile << decimal_to_binary(_uint64[i][j]);
      }
      outfile << "\n";
    }
    outfile.close();
  }
#endif


// Write 2D binary array to binary file
template<typename T>
inline void write_results_bin(T** rst, const int ncol, const int nrow, const char* filename) {
  int i, j;
  ofstream bin_file(filename, std::ios::binary);
  printf("Writing:\t%s\n", filename);
  for( i = 0; i < ncol; i++) {
    for( j = 0; j < nrow; j++) {
      bin_file.write((char*) &rst[i][j], sizeof(T));
      // printf("%d %d %d\n", i, j, rst[i][j]);
    }
  }
  bin_file.close();
}

// Read binary file into 2D binary array
template<typename T>
inline void read_results_bin(T** rst, const int nrow, const int ncol, const char* filename) {
  int i, j;
  ifstream bin_file(filename, std::ios::binary);
  printf("Filename:\t%s\n", filename);
  for( i = 0; i < ncol; i++) {
    for( j = 0; j < nrow; j++) {
      bin_file.read((char*) &rst[i][j], sizeof(T));
      //printf("%d %d %d\n", i, j, rst[i][j]);
    }
  }
  bin_file.close();
}

template<typename T>
BSFResult check_results(const T** results, const unsigned lcol, const unsigned qcol) {
  unsigned i, j;
  unsigned ncol = 0;
  if (qcol == 0 ) ncol = lcol;

  uint64_t nonzeros = 0ull, total = ncol==0?(uint64_t)lcol*(uint64_t)qcol:(uint64_t)(ncol*(ncol-1)/2);
  unsigned min = UINT_MAX, max = 0;
  if (ncol>0) {
    for (i = 0; i < ncol-1; i++) {
      for(j = i + 1; j < ncol; j++) {
        if (results[i][j] > 0) nonzeros++;
        if (min > results[i][j]) min = results[i][j];
        if (max < results[i][j]) max = results[i][j];
      }
    }  
  } else {
    for (i = 0; i < lcol; i++) {
      for(j = 0; j < qcol; j++) {
        if (results[i][j] > 0) nonzeros++;
        if (min > results[i][j]) min = results[i][j];
        if (max < results[i][j]) max = results[i][j];
      }
    }
  }
  
  double percentage = 100.0*nonzeros/total;
  printf("min: %d, max: %d\n", min, max);
  printf("%llu non-zeros/%llu\t(%.2lf%%)\n", nonzeros, total, percentage);
  
  BSFResult result;
  result.total = total;
  result.nonzeros = nonzeros;
  result.max = max;
  result.min = min;
  result.nonzero_percent = percentage;

  // histograms
  result.histo = new unsigned[max+1];
  for (i = 0; i < max+1; i++) result.histo[i] = 0;
  
  if (ncol>0) {
    for (i = 0; i < ncol-1; i++) {
      for(j = i + 1; j < ncol; j++) {
        result.histo[results[i][j]]++;
      }
    }
  } else {
    for (i = 0; i < lcol; i++) {
      for(j = 0; j < qcol; j++) {
        result.histo[results[i][j]]++;
      }
    }
  }
  

  return result;
}

template<typename T>
void write_results_tuple(const T** results, const unsigned lcol, const unsigned qcol, char* rfile, const unsigned threshold) {
  unsigned i, j;
  ofstream outfile;
  // char buf[128];
  // sprintf(buf, "tuple_%s", rfile);
  outfile.open(rfile);
  for (i = 0; i < lcol; i++){
    for(j = 0; j < qcol; j++){
      if (results[i][j] >= threshold) {
        outfile << i << "," << j << "," << (int)results[i][j] << "\n";
      }
    }
  }
  outfile.close();
}

template<typename T>
void write_results_tuple(const T** results, const unsigned lcol, const unsigned qcol, char* rfile) {
  write_results_tuple<T>(results, lcol, qcol, rfile, 1);
}


template<typename T>
void write_results_table(const T** results, const unsigned lcol, const unsigned qcol, char* rfile) {
  unsigned i, j;
  ofstream outfile;
  char buf[128];
  sprintf(buf, "table_%s", rfile);
  outfile.open(buf);
  for (i = 0; i < lcol; i++){
    for(j = 0; j < qcol; j++){
      sprintf(buf, "%c", '0'+results[i][j]);
      outfile << buf;
    }
    outfile << "\n";
  }
  outfile.close();
}

void write_results_histogram(BSFResult rst, char* rfile, char* dir) {
  unsigned i;
  ofstream outfile;
  char buf[1024];
  sprintf(buf, "%shisto_%s", dir, rfile);
  outfile.open(buf, ios_base::app | ios_base::out);

  sprintf(buf, "%ld\t%d\t%d\t%d\t%llu\t%llu\t%lf", 
    rst.time,
    rst.msec,
    rst.min,
    rst.max,
    rst.total,
    rst.nonzeros,
    rst.nonzero_percent
  );

  for (i = 0; i < rst.max+1; i++){
    outfile << "\t" << rst.histo[i];
  }
  outfile << "\n";
  outfile.close();
}

void write_results_histogram(BSFResult rst, char* rfile, char* dir, unsigned ci, unsigned cj) {
  unsigned i;
  ofstream outfile;
  char buf[1024];
  sprintf(buf, "%shisto_%s", dir, rfile);
  outfile.open(buf, ios_base::app | ios_base::out);

  sprintf(buf, "%ld\t%d\t%d\t%d\t%d\t%d\t%llu\t%llu\t%lf", 
    rst.time,
    ci,
    cj,
    rst.msec,
    rst.min,
    rst.max,
    rst.total,
    rst.nonzeros,
    rst.nonzero_percent
  );
  outfile << buf;

  for (i = 0; i < rst.max+1; i++){
    outfile << "\t" << rst.histo[i];
  }
  outfile << "\n";
  outfile.close();
}

static PyObject* analysis(PyObject *self, PyObject *args)
{
  PyArrayObject *cin, *cout;  // The python objects to be extracted from the args
  
  char *s = "results.txt";
  int size;

  npy_intp dimsout[2];
  npy_intp dims[2];

  /* Parse tuples separately since args will differ between C fcns */
    if (!PyArg_ParseTuple(args, "O!|s#", &PyArray_Type, &cin, &s, &size))  return NULL;
    if (NULL == cin)  return NULL;
    /* Check that objects are 'uint64' type and vectors
         Not needed if python wrapper function checks before call to this routine */
    if (not_2Duint64(cin)) return NULL;

    /* Get the dimensions of the input */
    int ncol=dims[0]=cin->dimensions[0];
    int nrow=dims[1]=cin->dimensions[1];

    printf("filename: %s, size: %d\n", s, size);
    printf("ncol: %d, nrow: %d\n", ncol, nrow);

    dimsout[0]=dimsout[1]=ncol;

    /* Make a new uint64 array of same dims */
    
    cout = (PyArrayObject *) PyArray_ZEROS(2, dimsout, NPY_UINT, 0);

    uint64_t** up_uint64 = py2D_to_Carrayptrs<uint64_t>(cin);
    printf("Allocating the uint64 matrix...\n");
    unsigned** rst = py2D_to_Carrayptrs<unsigned>(cout);
    printf("Allocating the result matrix...: %d-by-%d\n", ncol, ncol);
  
  #ifdef _OPENMP
  printf("==================OPENMP====================\n");
  #endif
  printf("Bits: %d =================================\n", nrow);
  #ifdef DEBUG
  writeMatrix((const uint64_t**)up_uint64, ncol, nrow);
  write_binary(up_uint64, nrow, ncol, "debug_matrix.bin");
  #endif
  clock_t start = clock(), diff;
  BSF::BSFCore::analysis((const uint64_t**)up_uint64, rst, ncol, nrow*64);
  diff = clock() - start;
  int msec = diff * 1000 / CLOCKS_PER_SEC;
  printf("Runtime:\t%d msec\n", msec);
  
  BSFResult bsf_rst = check_results<unsigned>((const unsigned**)rst, ncol, 0);
  // write_results_table<unsigned>(rst, ncol, ncol);
  // write_results_tuple<unsigned>(rst, ncol, ncol);
  // write_results_bin<unsigned>(rst, ncol, ncol);
  write_results_histogram(bsf_rst, s, "");

  free(up_uint64);

  return PyArray_Return(cout);
}

static PyObject* analysis_with_chunk(PyObject *self, PyObject *args)
{
  PyArrayObject *cin;  // The python objects to be extracted from the args
  
  char *rfile = "results.txt";
  char *dir = "";
  int chunkSize;

  npy_intp dimsout[2];
  npy_intp dims[2];

  /* Parse tuples separately since args will differ between C fcns */
  if (!PyArg_ParseTuple(args, "O!i|ss", &PyArray_Type, &cin, &chunkSize, &rfile, &dir))  return NULL;
  if (NULL == cin)  return NULL;
  /* Check that objects are 'uint64' type and vectors
       Not needed if python wrapper function checks before call to this routine */
  if (not_2Duint64(cin)) return NULL;

  /* Get the dimensions of the input */
  int ncol=dims[0]=cin->dimensions[0];
  int nrow=dims[1]=cin->dimensions[1];

  printf("filename: %s, chunk size: %d\n", rfile, chunkSize);
  printf("dir: %s\n", dir);
  printf("ncol: %d, nrow: %d\n", ncol, nrow);
  #ifdef _OPENMP
  printf("==================OPENMP====================\n");
  #endif

  uint64_t** up_uint64 = py2D_to_Carrayptrs<uint64_t>(cin);
  printf("Allocate the uint64 matrix...\n");

  // malloc once, and reuse
  unsigned** rst = new unsigned*[chunkSize];
  for (int k = 0; k < chunkSize; k++) rst[k] = new unsigned[chunkSize];

  //// chunks
  unsigned i, j;
  int remainder = ncol%chunkSize;
  int nChunk = remainder==0?(int)(ncol/chunkSize):(int)(ncol/chunkSize)+1;
  printf("no. of chunks: %d\n", nChunk);
  for (i = 0; i < nChunk; i++) {
    for (j = i; j < nChunk; j++) {
      BSFResult bsf_rst;
      int msec = 0;
      // set the size of a chunk of the output matrix
      if (i < nChunk-1) dimsout[0] = chunkSize;
      else dimsout[0] = remainder==0?chunkSize:remainder;
      if (j < nChunk-1) dimsout[1] = chunkSize;
      else dimsout[1] = remainder==0?chunkSize:remainder;

      unsigned x1 = i*chunkSize, x2 = i*chunkSize+dimsout[0];
      unsigned y1 = j*chunkSize, y2 = j*chunkSize+dimsout[1];

      // free?
      for (int k = 0 ; k < dimsout[0]; k++)
        for (int kk = 0 ; kk < dimsout[1]; kk++)
          rst[k][kk] = 0;

      printf("============================================\n");
      printf("Allocating the result matrix...: %ld-by-%ld\n", dimsout[0], dimsout[1]);
      printf("Chunk [%d,%d] and [%d,%d]\n", x1, x2, y1, y2);
      if(i == j) {
        // half matrix
        if (dimsout[0] != dimsout[1]) printf("ERROR: a chunk index is i==j but dimensions are not cubic.\n");
        clock_t start = clock(), diff;
        BSF::BSFCore::analysis_with_chunks((const uint64_t**)up_uint64, rst, x1, x2, nrow*64);
        diff = clock() - start;
        msec = diff * 1000 / CLOCKS_PER_SEC;
        printf("Runtime:\t%d msec\n", msec);
        
        bsf_rst = check_results<unsigned>((const unsigned**)rst, dimsout[0], 0);
      } else {
        // full matrix
        clock_t start = clock(), diff;
        BSF::BSFCore::analysis_with_chunks((const uint64_t**)up_uint64, rst, x1, x2, y1, y2, nrow*64);
        diff = clock() - start;
        msec = diff * 1000 / CLOCKS_PER_SEC;
        printf("Runtime:\t%d msec\n", msec);
        bsf_rst = check_results<unsigned>((const unsigned**)rst, dimsout[0], dimsout[1]);
      }

      bsf_rst.time = time(NULL);
      bsf_rst.msec = msec;
      write_results_histogram(bsf_rst, rfile, dir, i, j);

      char buf[1024];
      sprintf(buf, "%sbin_%d_%d_%d_%ld_%ld_%s.bin", dir, chunkSize, i, j, dimsout[0], dimsout[1], rfile);
      write_results_bin<unsigned>(rst, dimsout[0], dimsout[1], buf);
      
      printf("============================================\n");
    }
  }
  // free memory
  for(int k = 0; k < chunkSize; k++) delete [] rst[k];
  delete [] rst;

  free_Carrayptrs(up_uint64);
  return Py_BuildValue("i", 1);
}

static PyObject* analysis_with_query(PyObject *self, PyObject *args)
{
  PyArrayObject *clib, *cquery;
  
  char *rfile = "results.txt";
  char *dir = "";
  // int chunkSize;

  // npy_intp dimsout[2];
  npy_intp lib_dims[2];
  npy_intp q_dims[2];

  /* Parse tuples separately since args will differ between C fcns */
  if (!PyArg_ParseTuple(args, "O!O!|ss", &PyArray_Type, &clib, &PyArray_Type, &cquery, &rfile, &dir))  return NULL;
  if (NULL == clib)  return NULL;
  if (NULL == cquery)  return NULL;
  /* Check that objects are 'uint64' type and vectors
       Not needed if python wrapper function checks before call to this routine */
  if (not_2Duint64(clib)) return NULL;
  if (not_2Duint64(cquery)) return NULL;

  /* Get the dimensions of the input */
  int lcol=lib_dims[0]=clib->dimensions[0];
  int lrow=lib_dims[1]=clib->dimensions[1];

  int qcol=q_dims[0]=cquery->dimensions[0];
  int qrow=q_dims[1]=cquery->dimensions[1];

  /* different size of vectors */
  if (lrow != qrow) return NULL;
  int nrow = lrow;

  printf("filename: %s, chunk size: N/A\n", rfile);
  printf("dir: %s\n", dir);
  printf("lcol: %d, lrow: %d\n", lcol, lrow);
  printf("qcol: %d, qrow: %d\n", qcol, qrow);
  #ifdef _OPENMP
  printf("==================OPENMP====================\n");
  #endif

  uint64_t** lmat = py2D_to_Carrayptrs<uint64_t>(clib);
  uint64_t** qmat = py2D_to_Carrayptrs<uint64_t>(cquery);
  printf("Allocate the uint64 matrix...\n");

  // malloc lcol-by-qcol
  unsigned** rst = new unsigned*[lcol];
  for (int k = 0; k < lcol; k++) rst[k] = new unsigned[qcol];

  clock_t start = clock(), diff;
  BSF::BSFCore::analysis_with_query((const uint64_t**)lmat, (const uint64_t**)qmat, rst, 0, lcol, 0, qcol, nrow*64);
  diff = clock() - start;
  int msec = diff * 1000 / CLOCKS_PER_SEC;
  printf("Runtime:\t%d msec\n", msec);
  
  BSFResult bsf_rst = check_results<unsigned>((const unsigned**)rst, lcol, qcol);

  bsf_rst.time = time(NULL);
  bsf_rst.msec = msec;
  write_results_histogram(bsf_rst, rfile, dir, 0, 0);

  char buf[1024];
  sprintf(buf, "%sbin_%s.bin", dir, rfile);
  write_results_bin<unsigned>(rst, lcol, qcol, buf);
  printf("============================================\n");
  
  for(int k = 0; k < lcol; k++) delete [] rst[k];
  delete [] rst;

  free_Carrayptrs(lmat);
  free_Carrayptrs(qmat);
  return Py_BuildValue("i", 1);
}

static PyObject* read_bin_file(PyObject *self, PyObject *args)
{
  PyArrayObject *cout;  // The python objects to be extracted from the args
  
  char *rfile = "results.txt";
  char *dir = "";
  int ncol, nrow;
  npy_intp dimsout[2];

  /* Parse tuples separately since args will differ between C fcns */
  if (!PyArg_ParseTuple(args, "iiss", &ncol, &nrow, &rfile, &dir))  return NULL;
  
  printf("filename: %s, dir: %s\n", rfile, dir);
  printf("ncol: %d, nrow: %d\n", ncol, nrow);
  dimsout[0] = ncol;
  dimsout[1] = nrow;
  cout = (PyArrayObject *) PyArray_ZEROS(2, dimsout, NPY_UINT, 0);
  unsigned** rst = py2D_to_Carrayptrs<unsigned>(cout);
  char buf[1024];
  sprintf(buf, "%s%s", dir, rfile);

  // unsigned** rst = new unsigned*[dimsout[0]];
  // for (int k = 0; k < dimsout[0]; k++) rst[k] = new unsigned[dimsout[1]];

  read_results_bin<unsigned>(rst, nrow, ncol, buf);
  // free_Carrayptrs(rst);
  // return Py_BuildValue("i", 1);
  return PyArray_Return(cout);
}

static PyObject* fetch_tuples(PyObject *self, PyObject *args)
{
  // PyArrayObject *cout;  // The python objects to be extracted from the args
  
  char *rfile = "results.txt";
  char *dir = "";
  int ncol, nrow, threshold;
  npy_intp dimsout[2];

  /* Parse tuples separately since args will differ between C fcns */
  if (!PyArg_ParseTuple(args, "iissi", &ncol, &nrow, &rfile, &dir, &threshold))  return NULL;
  
  printf("filename: %s, dir: %s\n", rfile, dir);
  printf("ncol: %d, nrow: %d\n", ncol, nrow);
  dimsout[0] = ncol;
  dimsout[1] = nrow;
  char buf[1024];
  sprintf(buf, "%s%s", dir, rfile);

  unsigned** rst = new unsigned*[dimsout[0]];
  for (int k = 0; k < dimsout[0]; k++) rst[k] = new unsigned[dimsout[1]];
  read_results_bin<unsigned>(rst, nrow, ncol, buf);
  sprintf(buf, "%stuple_%s.txt", dir, rfile);
  write_results_tuple<unsigned>((const unsigned**)rst, nrow, ncol, buf, (unsigned)threshold);

  return Py_BuildValue("i", 1);
}

static PyMethodDef BSFMethods[] = {
  {"strlen", test, METH_VARARGS, "count a string length."},
  {"test", func, METH_VARARGS, "count a string length."},
  {"analysis", analysis, METH_VARARGS, "count a string length."},
  {"analysis_with_chunk", analysis_with_chunk, METH_VARARGS, "count a string length."},
  {"analysis_with_query", analysis_with_query, METH_VARARGS, "count a string length."},
  {"read_bin_file", read_bin_file, METH_VARARGS, "count a string length."},
  {"fetch_tuples", fetch_tuples, METH_VARARGS, "count a string length."},
  {NULL, NULL, 0, NULL}
};

static struct PyModuleDef bsfmodule = {
  PyModuleDef_HEAD_INIT,
  "bsf",
  "It's a BSF module.",
  -1, BSFMethods
};

PyMODINIT_FUNC PyInit_bsf(void){
  import_array();
  return PyModule_Create(&bsfmodule);
}