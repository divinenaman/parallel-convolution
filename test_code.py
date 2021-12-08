#stdout_buf = StringIO(result)
#result = np.frombuffer(result,dtype='int32')
#result = int.from_bytes(result, byteorder='little', signed=True)
#result = memoryview(result[0]).cast('l')

# so_file = os.path.join(os.getcwd(),"conv.so")

# cbind = ctypes.CDLL(so_file)


# cbind.parallel_convolution.restype = ndpointer(dtype=ctypes.c_int, shape=(5,5))
# cbind.parallel_convolution.argtypes = [ctypes.POINTER(ctypes.c_int), ctypes.c_int]


# A = np.asarray([
#     [199,208,233,4,5],
#     [200,140,10,212,13],
#     [150,65,100,101,101],
#     [140,50,255,90,10],
#     [224,69,89,210,211]
# ])

# array = A.ctypes.data_as(ctypes.POINTER(ctypes.c_int))
# res = cbind.parallel_convolution(array, 5)

# print(res)