COMPILE = gcc -Wall


INCLUDE_DIR = ..
CFLAGS = -O2 -ffast-math -msse2 -funroll-loops -I $(INCLUDE_DIR)
# -fomit-frame-pointer 

ALL: clean test_matrix_multiplication test_matrix_transform test_Mat4Copy test_transform_model_to_clip


test_matrix_multiplication: test_matrix_multiplication.c ../matrix_multiplication.c
	$(COMPILE) test_matrix_multiplication.c ../matrix_multiplication.c -o test_matrix_multiplication -x assembler\-with\-cpp MatrixMultiply4x4_ASM.s $(CFLAGS)

test_matrix_transform: test_matrix_transform.c ../matrix_multiplication.c
	$(COMPILE) test_matrix_transform.c ../matrix_multiplication.c -o test_matrix_transform  $(CFLAGS)

test_Mat4Copy: 
	$(COMPILE) test_Mat4Copy.c -o test_Mat4Copy $(CFLAGS)

test_transform_model_to_clip:
	$(COMPILE) test_transform_model_to_clip.c ../matrix_multiplication.c -o test_transform_model_to_clip $(CFLAGS)


clean:
	rm -f *.o test_matrix_multiplication test_Mat4Copy test_transform_model_to_clip test_matrix_transform 
#main:main.o
#	$(COMPILE) -o $(TARGETS) main.o
#.c.o:
#	$(COMPILE) -c $< -o $@
#make
