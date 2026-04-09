################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/Components/l3gd20/l3gd20.c 

OBJS += \
./Drivers/Components/l3gd20/l3gd20.o 

C_DEPS += \
./Drivers/Components/l3gd20/l3gd20.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/Components/l3gd20/%.o Drivers/Components/l3gd20/%.su Drivers/Components/l3gd20/%.cyclo: ../Drivers/Components/l3gd20/%.c Drivers/Components/l3gd20/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F429xx -c -I../Core/Inc -I"C:/Users/T00691725/OneDrive - Thompson Rivers University/STM32CubeMx/Final Project 4/Drivers/STM32F429I-Discovery" -I"C:/Users/T00691725/OneDrive - Thompson Rivers University/STM32CubeMx/Final Project 4/Drivers/Components/ili9341" -I../USB_HOST/App -I../USB_HOST/Target -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/tomsp/OneDrive/Documents/Programming/CENG 3020/Final Project/Drivers/STM32F429I-Discovery" -I"C:/Users/tomsp/OneDrive/Documents/Programming/CENG 3020/Final Project/Drivers/Components" -I"C:/Users/T00691725/OneDrive - Thompson Rivers University/STM32CubeMx/Final Project 4/Drivers/Fonts" -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-Components-2f-l3gd20

clean-Drivers-2f-Components-2f-l3gd20:
	-$(RM) ./Drivers/Components/l3gd20/l3gd20.cyclo ./Drivers/Components/l3gd20/l3gd20.d ./Drivers/Components/l3gd20/l3gd20.o ./Drivers/Components/l3gd20/l3gd20.su

.PHONY: clean-Drivers-2f-Components-2f-l3gd20

