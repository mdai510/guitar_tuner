################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/ThirdParty/kiss-fft/kiss_fft.c \
../Core/ThirdParty/kiss-fft/kiss_fftr.c 

OBJS += \
./Core/ThirdParty/kiss-fft/kiss_fft.o \
./Core/ThirdParty/kiss-fft/kiss_fftr.o 

C_DEPS += \
./Core/ThirdParty/kiss-fft/kiss_fft.d \
./Core/ThirdParty/kiss-fft/kiss_fftr.d 


# Each subdirectory must supply rules for building sources it contributes
Core/ThirdParty/kiss-fft/%.o Core/ThirdParty/kiss-fft/%.su Core/ThirdParty/kiss-fft/%.cyclo: ../Core/ThirdParty/kiss-fft/%.c Core/ThirdParty/kiss-fft/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_NUCLEO_64 -DUSE_HAL_DRIVER -DSTM32G474xx -c -I../Core/ThirdParty/kiss-fft -I../Core/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/STM32G4xx_Nucleo -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-ThirdParty-2f-kiss-2d-fft

clean-Core-2f-ThirdParty-2f-kiss-2d-fft:
	-$(RM) ./Core/ThirdParty/kiss-fft/kiss_fft.cyclo ./Core/ThirdParty/kiss-fft/kiss_fft.d ./Core/ThirdParty/kiss-fft/kiss_fft.o ./Core/ThirdParty/kiss-fft/kiss_fft.su ./Core/ThirdParty/kiss-fft/kiss_fftr.cyclo ./Core/ThirdParty/kiss-fft/kiss_fftr.d ./Core/ThirdParty/kiss-fft/kiss_fftr.o ./Core/ThirdParty/kiss-fft/kiss_fftr.su

.PHONY: clean-Core-2f-ThirdParty-2f-kiss-2d-fft

