################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/oldCode/RetiredCode.c \
../Core/oldCode/memFunc.c 

OBJS += \
./Core/oldCode/RetiredCode.o \
./Core/oldCode/memFunc.o 

C_DEPS += \
./Core/oldCode/RetiredCode.d \
./Core/oldCode/memFunc.d 


# Each subdirectory must supply rules for building sources it contributes
Core/oldCode/%.o Core/oldCode/%.su: ../Core/oldCode/%.c Core/oldCode/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-oldCode

clean-Core-2f-oldCode:
	-$(RM) ./Core/oldCode/RetiredCode.d ./Core/oldCode/RetiredCode.o ./Core/oldCode/RetiredCode.su ./Core/oldCode/memFunc.d ./Core/oldCode/memFunc.o ./Core/oldCode/memFunc.su

.PHONY: clean-Core-2f-oldCode

