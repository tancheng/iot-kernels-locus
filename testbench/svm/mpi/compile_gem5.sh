rm *.out
arm-linux-gnueabi-g++ *.c -I ../../lib -I ../../lib/m5 ../../lib/m5/m5op_arm.S --static -o a_arm.out
