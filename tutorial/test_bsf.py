import unittest
import bsf
import numpy as np

class BSFTestMethods(unittest.TestCase):
    def test_compare_library_and_library(self):
        # generate random matrix with uint64
        nsign = 9  # the number of signatures
        nquery = 5  # the number of queries
        length = 12  # a length of a signature
        # a library matrix
        lmat = np.random.randint(2, size=(nsign, length)).astype(np.uint64)
        print('Randomly generated library:', lmat)
        # Run BSF to compute the pairwise similiarity of all the pairs of matrix
        bsf.analysis_with_chunk(lmat, nsign, 'bsf.txt', '')
        # read a result table (binary file)
        with open("bin_9_0_0_9_9_bsf.txt.bin", "rb") as f:
            rst_mat = np.frombuffer(f.read(), dtype=np.uint32).reshape((nsign, nsign))
        print('Similarity between datasets in the library:', rst_mat)
    def test_compare_library_and_query(self):
        # generate random matrix with uint64
        nsign = 9  # the number of signatures
        nquery = 5  # the number of queries
        length = 12  # a length of a signature
        # a library matrix
        lmat = np.random.randint(2, size=(nsign, length)).astype(np.uint64)
        # generate query matrix
        qmat = np.random.randint(2, size=(nquery, length)).astype(np.uint64)
        print('Randomly generated library:', lmat)
        print('Randomly generated query:', qmat)
        # Run BSF to compute the pairwise similiarity of all the pairs of matrix
        bsf.analysis_with_query(lmat, qmat, 'qbsf.txt', '')
        # read a result table (binary file)
        with open("bin_qbsf.txt.bin", "rb") as f:
            rst_mat = np.frombuffer(f.read(), dtype=np.uint32).reshape((nsign, nquery))
        print('Similarity between the library and the query:', rst_mat)

if __name__ == '__main__':
    unittest.main()