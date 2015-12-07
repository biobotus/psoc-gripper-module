/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

#include <Z_Axis.h>

/*----Private functions declarations----*/
CY_ISR(Z_Axis_Bumper_ISR);
CY_ISR(Z_Axis_Move_ISR);
void Z_Axis_Bumper_Stops();

/*---- State Machine definition ----*/
#define WAITING_FOR_BUMPER1 0
#define GOING_AWAY 1
#define WAITING_FOR_BUMPER2 2
#define HOME 3


/*----Private variables---*/
uint8 Z_axis_state;
uint16 Z_pos;
uint16 screw_lead = 80;           // in hundreds of micrometers/turn
uint16 motor_steps_turn = 200;      // steps/turn for motor
uint16 f_ustepping = 4;             // microstepping factor (1,2,4,8,16)
uint8 motor_direction = 1;          // 1 = up, 0 = down
uint8 Z_axis_bumper_count = 0;
const uint8 motor_up = 1;
const uint8 motor_down = 0;
const uint16 max_distance = 3490;   // hundreds of micrometers

void Z_Axis_Init()
{
     //*****Init variables******///
    Z_Axis_Clock_Start();
    
    //Motor speed init
    Z_Motor_Speed_Start();
    //Distance init
    Z_Step_Dist_Start();
    //Motor power init
    Z_Axis_Power_Start();
    
    //Bumper interrupt initialization
    //Z_Axis_Bumper_ISR_StartEx(Z_Axis_Bumper_ISR);
    //Z_Axis_Bumper_Port_ClearInterrupt();
    //Z_Axis_Bumper_ISR_ClearPending();
    
    //Movement interrupt initialization
    Z_Axis_Move_ISR_StartEx(Z_Axis_Move_ISR);
    Z_Axis_Move_ISR_ClearPending();
    
    switch (f_ustepping) {
        case 1: // 1 ustepping
        {
            Z_Axis_StepSizeA_Write(0);
            Z_Axis_StepSizeB_Write(0);
            break;
        }
        
        case 2: // 1/2 ustepping
        {
            Z_Axis_StepSizeA_Write(1);
            Z_Axis_StepSizeB_Write(0);
            break;
        }
        
        case 4: // 1/4 ustepping
        {
            Z_Axis_StepSizeA_Write(0);
            Z_Axis_StepSizeB_Write(1);
            break;
        }
        
        case 8: // 1/8 ustepping
        {
            //Allegro chip can't make 1/8 ustepping
            break;
        }
        
        case 16: // 1/16 ustepping
        {
            Z_Axis_StepSizeA_Write(1);
            Z_Axis_StepSizeB_Write(1);
            break;
        }
        default:
        {
            break;
        }
    }
    
    Z_Axis_Direction_Write(motor_direction);
    Z_Axis_Enable_Write(0);
    Z_axis_state = WAITING_FOR_BUMPER1;
    
    Z_Axis_Home();
}

void Z_Axis_Home() {
    //Verifies if already on bumper, otherwise gets to bumper
    if (!(Z_Axis_Bumper_Port_Read())){
        Z_Axis_Bumper_ISR_StartEx(Z_Axis_Bumper_ISR);
        Z_Axis_Bumper_Port_ClearInterrupt();
        Z_Axis_Bumper_ISR_ClearPending();
        // Activate motor to get to bumper
        Z_Axis_Direction_Write(motor_up);
        Z_Step_Dist_WriteCompare(max_distance*((motor_steps_turn*f_ustepping)/screw_lead));
        Z_GO_Write(1); //Motor starts
    }
    else {
        sendDataToCAN(CAN_INSTRUCTION_SET_Z_AXIS_HOME, CAN_HARDWARE_FILTER_ID, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    }
}

// Initialization for
void Z_Axis_Bumper_Stops()
{   
    switch (Z_axis_state){
        case WAITING_FOR_BUMPER1:
        {
            Z_Step_Dist_WriteCompare(0);
            Z_pos = 0;
            Z_Axis_Move_ISR_ClearPending();
            Z_Axis_GoTo(200);
            Z_axis_state = GOING_AWAY;
            break;
        }
        
        case GOING_AWAY:
        {
            Z_Step_Dist_WriteCompare(0);
            Z_pos = 0;
            Z_Axis_Move_ISR_ClearPending();
            Z_Axis_GoTo(200);
            Z_axis_state = GOING_AWAY;
            break;
        }
        
        case WAITING_FOR_BUMPER2:
        {
            Z_axis_state = WAITING_FOR_BUMPER1;
            Z_Step_Dist_WriteCompare(0);
            Z_pos = 0;
            Z_Axis_Move_ISR_ClearPending();
            sendDataToCAN(CAN_INSTRUCTION_SET_Z_AXIS_HOME, CAN_HARDWARE_FILTER_ID, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
            CyDelay(200);
            Z_Axis_Bumper_ISR_ClearPending();
            Z_Axis_Bumper_Port_ClearInterrupt();
            Z_Axis_Bumper_ISR_Stop();
            break;
        }
        default:
        {
            break;
        }
    }
}

void Z_Axis_GoTo(uint16 position){
    uint16 travel;
    uint32 steps_to_do;
    if (position > max_distance){position = max_distance;}
    if (position < Z_pos) {
        travel = Z_pos - position;
        steps_to_do = travel*((motor_steps_turn*f_ustepping)/screw_lead);
        motor_direction = motor_up;
    }
    
    else if (position > Z_pos) {
        travel = position - Z_pos;
        steps_to_do = travel*((motor_steps_turn*f_ustepping)/screw_lead);
        motor_direction = motor_down;
    }
    
    else {
        travel = 0;
        steps_to_do = 0;
        sendDataToCAN(CAN_INSTRUCTION_SET_Z_AXIS_POSITION, CAN_HARDWARE_FILTER_ID, 0x00, 0x00, 0x00, 0x00, (uint8)(Z_pos >> 8), (uint8)Z_pos);
    }
    Z_pos = position;
    // Setting up motor direction
    Z_Axis_Direction_Write(motor_direction);
    Z_Step_Dist_WriteCompare(steps_to_do);
    Z_GO_Write(1);
}

CY_ISR(Z_Axis_Bumper_ISR){
    Z_Axis_Bumper_ISR_ClearPending();
    Z_Axis_Bumper_Port_ClearInterrupt();
    Z_Axis_Bumper_Stops();
}

CY_ISR(Z_Axis_Move_ISR){
    switch (Z_axis_state){
        case WAITING_FOR_BUMPER1:
        {
            // Send back on CAN that the action is done
            sendDataToCAN(CAN_INSTRUCTION_SET_Z_AXIS_POSITION, CAN_HARDWARE_FILTER_ID, 0x00, 0x00, 0x00, 0x00, (uint8)(Z_pos >> 8), (uint8)Z_pos);
            break;
        }
        
        case GOING_AWAY:
        {
            // Calls back for homing
            Z_axis_state = WAITING_FOR_BUMPER2;
            Z_Axis_Home();
            break;
        }
        
        case WAITING_FOR_BUMPER2:
        {
            break;
        }
        default:
        {
            break;
        }
    }
    Z_Axis_Move_ISR_ClearPending();
}
/* [] END OF FILE */