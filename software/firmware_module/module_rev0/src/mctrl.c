/* Copyright (C) 2025 Dennis Gunia - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the  AGPL-3.0 license.
 *
 * https://www.dennisgunia.de
 * https://github.com/dennis9819/splitflap_v1
 */


#include "mctrl.h"

// Motor driver steps definition. Reverse for direction change.
uint8_t motor_steps[4] = {
    0b00000001,
    0b00000010,
    0b00000100,
    0b00001000,
};

uint8_t step_index = 0;             // current index in motor_steps
uint8_t target_flap = 0;            // target flap
uint16_t absolute_pos = 0;          // absolute position in steps
uint16_t steps_since_home = 0;      // steps since last home signal

// homing util variables
uint8_t homing = 0;   // current homing step
uint8_t lastSens = 0; // home sonsor signal from last tick

// counter for auto powersaving
uint8_t ticksSinceMove = 0;

// value to goto after the current is reached. 255 = NONE.
uint8_t afterRotation = STEPS_AFTERROT;

int16_t *delta_err;

// error and status flags
uint8_t sts_flag_errorTooBig = 0; // last home signal too early or too late
uint8_t sts_flag_noHome = 0;      // no home signal detected. Wheel stuck
uint8_t sts_flag_fuse = 0;        // blown fuse detcted
uint8_t sts_flag_pwrdwn = 0;      // device is powered down by controller
uint8_t sts_flag_failsafe = 0;    // device is powered down for safety reasons
uint8_t sts_flag_busy = 0;        // device is busy
// voltage monitoring variables
uint16_t currentVoltage = 0;      // current ADC reading
uint8_t currentFaultReadings = 0; // ticks with faulty readings (too many will
                                  // trip pwrdwn and sts_flag_fuse)

int STEPS_OFFSET = 0;
// initialize motor controller
void mctrl_init(int cal_offset)
{
    if (cal_offset < 800){
        STEPS_OFFSET = STEPS_OFFSET_DEF;
    }else{
        STEPS_OFFSET = cal_offset;
    }
    DDRC = 0x0F;  // set all pins as outputs
    PORTC = 0x00; // set all to LOW

    DDRD &= ~(1 << PD3); // PD3 is input
    PORTD |= (1 << PD3); // PD3 pullup

    // setup adc
    ADMUX = 0x07;                                      // Aref, ADC7
    ADCSRA = (1 << ADEN) | (1 << ADSC) | (1 << ADPS1); // Enable ADC, Start first
                                                       // reading No freerunning, 8MHz
    while ((ADCSRA & (1 << ADSC)) > 0)
    {
    };
    // wait until first reading is complete,
    // to avoid error flag on first tick!

    // setup timer for ISR
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // CTC und Prescaler 64
    OCR1A = MISR_OCR1A;
    TIMSK = 1 << OCIE1A; // Timerinterrupts aktivieren
    homing = 1;
    delta_err = malloc(ERROR_DATASETS * sizeof(uint16_t));
    _delay_ms(MDELAY_STARTUP);
    sei();
}

// call when critical fail. Powers down motor and sets flags
void failSafe()
{
    sts_flag_failsafe = 1;
    PORTC = 0x00;
}

// read voltage non blocking (called every tick)
void readVoltage()
{
    currentVoltage = ADC;  // read last measurement
    ADMUX = 0x07;          // select ADC7
    ADCSRA |= (1 << ADSC); // trigger next reading
    if (currentVoltage < MVOLTAGE_LSTOP)
    { // if voltage is too low, fuse is probably broken
        currentFaultReadings++;
        if (currentFaultReadings > MVOLTAGE_FAULTRD)
        { // too many fault readings trigger failSafe
            sts_flag_fuse = 1;
            failSafe();
        }
    }
}

// MAIN service routine. Called by timer 1
ISR(TIMER1_COMPA_vect)
{
    readVoltage(); // read and check voltage
    if (sts_flag_pwrdwn == 1 || sts_flag_failsafe == 1)
    {
        return;
    } // if sts_flag_pwrdwn, STOP!
    if (steps_since_home > STEPS_PER_REV * MHOME_TOLERANCE)
    { // check if home is missing for too long
        // home missing error. Wheel probably stuck or power fail
        sts_flag_noHome = 1;
        failSafe();
    }
    else if (homing == 1)
    { // Homing procedure 1. step: move out of home
        if ((PIND & (1 << PD3)) > 0)
        {
            homing = 2;
        }
        else
        {
            mctrl_step();
        }
    }
    else if (homing == 2)
    { // Homing procedure 2. step: find magnet
        mctrl_step();
        if ((PIND & (1 << PD3)) == 0)
        {
            homing = 3;
            steps_since_home = 0;
            absolute_pos = STEPS_OFFSET;
            incrementCounter();
        }
    }
    else if (homing == 3)
    { // Homing procedure 3. step: apply offset
        if (absolute_pos <= 0)
        {
            homing = 0;
            absolute_pos = STEPS_OFFSET;    // set correct position again
        }
        mctrl_step();
        absolute_pos--;
    }
    else
    { // when no failsafe is triggered and homing is done
        // calculate target position
        uint16_t target_pos = (target_flap * STEPS_PER_FLAP) + STEPS_OFFSET;
        if (target_pos >= STEPS_PER_REV)
        {
            target_pos -= STEPS_PER_REV;
        }
        if (absolute_pos != target_pos)
        {
            // if target position is not reached, move motor
            ticksSinceMove = 0;
            mctrl_step();
            absolute_pos++;
            if (absolute_pos >= STEPS_PER_REV)
            {
                absolute_pos -= STEPS_PER_REV;
            }
            // detect home position
            if ((PIND & (1 << PD3)) == 0)
            {
                if (lastSens == 0)
                {
                    // new home transition
                    int16_t errorDelta =
                        (int16_t)(absolute_pos > (STEPS_PER_REV / 2) ? absolute_pos - STEPS_PER_REV : absolute_pos);
                    sts_flag_errorTooBig = (errorDelta > MHOME_ERRDELTA) || (errorDelta < -MHOME_ERRDELTA) ? 1 : 0;
                    // storeErr(errorDelta);
                    absolute_pos = 0;
                    steps_since_home = 0;
                    // increment rotations counter
                    incrementCounter();
                }
                lastSens = 1;
            }
            else
            {
                lastSens = 0;
            }
        }
        else
        { // if target position is reached
            if (afterRotation < (AMOUNTFLAPS + 5))
            { // if after rotation is set, apply it as new target
                target_flap = afterRotation;
                afterRotation = STEPS_AFTERROT;
            }
            else if (ticksSinceMove < 2)
            { // if motor has not been moved
                sts_flag_busy = 0;
            }
            else if (ticksSinceMove < MPWRSVG_TICKSTOP)
            { // if motor has not been moved
                ticksSinceMove++;
            }
            else
            {   // power off after 50 ticks
                // PORTC = 0x00; // turn off stepper
            }
        }
    }
    rc_tick(); // process counter tick, non-blocking
}

// TODO
void storeErr(int16_t error)
{
    int16_t *delta_err_tmp = malloc(ERROR_DATASETS * sizeof(uint16_t));
    memcpy(delta_err, delta_err_tmp + sizeof(uint16_t), ((ERROR_DATASETS - 2) * sizeof(uint16_t)));
    memcpy(&error, delta_err_tmp, sizeof(uint16_t));
    free(delta_err);
    delta_err = delta_err_tmp;
}
// TODO
void getErr(int16_t *error)
{
    memcpy(delta_err, error, (ERROR_DATASETS * sizeof(uint16_t)));
}

// return status flag
uint8_t getSts()
{
    uint8_t status = sts_flag_errorTooBig; // bit 0: delta too big
    status |= sts_flag_noHome << 1;        // bit 1: no home found
    status |= sts_flag_fuse << 2;          // bit 2: fuse blown
    status |= sts_flag_pwrdwn << 4;        // bit 4: device powered down
    status |= sts_flag_failsafe << 5;      // bit 5: failsafe active
    status |= sts_flag_busy << 6;          // bit 6: failsafe active
    if ((PIND & (1 << PD3)) == 0)
    {
        status |= (1 << 3);
    }
    return status;
}

// return voltage
uint16_t getVoltage()
{
    return currentVoltage;
}

// set target flap
void mctrl_set(uint8_t flap, uint8_t fullRotation)
{
    sts_flag_busy = 1;
    if (fullRotation == 0)
    {
        target_flap = flap;
    }
    else
    {
        target_flap = (target_flap + (STEPS_PER_FLAP - 1)) % STEPS_PER_FLAP;
        afterRotation = flap;
    }
}

// trigger home procedure
void mctrl_home()
{
    homing = 1;
}

// change motor power state
void mctrl_power(uint8_t state)
{
    if (state == 0)
    {
        sts_flag_pwrdwn = 1;
        PORTC = 0x00;
    }
    else
    {
        sts_flag_pwrdwn = 0;
        PORTC = motor_steps[step_index];
    }
}

// do stepper step (I/O)
void mctrl_step()
{
    step_index++;
    steps_since_home++;
    if (step_index > 3)
    {
        step_index = 0;
    }
    PORTC = motor_steps[step_index];
}
