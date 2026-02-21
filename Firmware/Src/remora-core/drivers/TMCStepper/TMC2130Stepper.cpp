#include "TMCStepper.h"
#include "TMC_MACROS.h"

// only software SPI supported

int8_t TMC2130Stepper::chain_length = 0;

TMC2130Stepper::TMC2130Stepper(const std::string& pinCS, float RS, const std::string& pinMOSI, const std::string& pinMISO, const std::string& pinSCK, int8_t link)
    : TMCStepper(RS),
      cs(nullptr),
      link_index(link) {

	cs = new Pin(pinCS, OUTPUT);

    TMC_SW_SPI = new SoftwareSPI(pinMOSI, pinMISO, pinSCK, pinCS, SPI_MODE_3, MSB_FIRST, MSB_FIRST_BYTE);

    defaults();

    switchCSpin(HIGH);
    TMC_SW_SPI->begin();

    if (link > chain_length)
      chain_length = link;
  }

TMC2130Stepper::TMC2130Stepper(const std::string& pinCS, const std::string& pinMOSI, const std::string& pinMISO, const std::string& pinSCK, int8_t link)
    : TMCStepper(default_RS),
      cs(nullptr),
      link_index(link) {

	cs = new Pin(pinCS, OUTPUT);

    TMC_SW_SPI = new SoftwareSPI(pinMOSI, pinMISO, pinSCK, pinCS, SPI_MODE_3, MSB_FIRST, MSB_FIRST_BYTE);

    defaults();

    switchCSpin(HIGH);
    TMC_SW_SPI->begin();

    if (link > chain_length)
      chain_length = link;
  }

void TMC2130Stepper::defaults() {
  //MSLUT0_register.sr = ???;
  //MSLUT1_register.sr = ???;
  //MSLUT2_register.sr = ???;
  //MSLUT3_register.sr = ???;
  //MSLUT4_register.sr = ???;
  //MSLUT5_register.sr = ???;
  //MSLUT6_register.sr = ???;
  //MSLUT7_register.sr = ???;
  //MSLUTSTART_register.start_sin90 = 247;
  PWMCONF_register.sr = 0x00050480;
}

__attribute__((weak))
void TMC2130Stepper::setSPISpeed(uint32_t speed) {
  spi_speed = speed;
}

__attribute__((weak))
void TMC2130Stepper::switchCSpin(bool state) {
  cs->set(state);
}

__attribute__((weak))
uint32_t TMC2130Stepper::read(uint8_t addressByte) {
    uint32_t out = 0UL;
    int8_t i = 1;

    if (!TMC_SW_SPI) return 0; // Ensure SPI instance is valid

    if (cs) 
    {
        cs->set(false); // Pull CS low
        TMC_SW_SPI->delay();
    }

    TMC_SW_SPI->transfer(addressByte);
    TMC_SW_SPI->transferEmptyBytes(4);

    while(i < link_index) {
        TMC_SW_SPI->transferEmptyBytes(5);
        i++;
    }

    if (cs) 
    {
        cs->set(true); // Pull CS high
        TMC_SW_SPI->delay();
        cs->set(false); // Pull CS low
        TMC_SW_SPI->delay();
    }

    status_response = TMC_SW_SPI->transfer(addressByte); // Send the address byte again
    out  = TMC_SW_SPI->transfer(0x00);
    out <<= 8;
    out |= TMC_SW_SPI->transfer(0x00);
    out <<= 8;
    out |= TMC_SW_SPI->transfer(0x00);
    out <<= 8;
    out |= TMC_SW_SPI->transfer(0x00);

    if (cs) 
    {
        cs->set(true); // Pull CS high
        TMC_SW_SPI->delay();
    }

    return out;
}

__attribute__((weak))
void TMC2130Stepper::write(uint8_t addressByte, uint32_t config) {
    addressByte |= TMC_WRITE;
    int8_t i = 1;

    if (!TMC_SW_SPI) return; // Ensure SPI instance is valid

    if (cs)
    {
        cs->set(false); // Pull CS low
        TMC_SW_SPI->delay();
    }

    status_response = TMC_SW_SPI->transfer(addressByte);
    TMC_SW_SPI->transfer(config>>24);
    TMC_SW_SPI->transfer(config>>16);
    TMC_SW_SPI->transfer(config>>8);
    TMC_SW_SPI->transfer(config);

    // Shift the written data to the correct driver in chain
    // Default link_index = -1 and no shifting happens
    while(i < link_index) {
        TMC_SW_SPI->transferEmptyBytes(5);
        i++;
    }

    if (cs)
    {
        cs->set(true); // Pull CS high
        TMC_SW_SPI->delay();
    }
}

void TMC2130Stepper::begin() {
  //set pins
  switchCSpin(HIGH);

  if (TMC_SW_SPI != nullptr) TMC_SW_SPI->begin();

  GCONF(GCONF_register.sr);
  CHOPCONF(CHOPCONF_register.sr);
  COOLCONF(COOLCONF_register.sr);
  PWMCONF(PWMCONF_register.sr);
  IHOLD_IRUN(IHOLD_IRUN_register.sr);

  toff(8); //off_time(8);
  tbl(1); //blank_time(24);
}

/**
 *  Helper functions
 */

bool TMC2130Stepper::isEnabled() { return !drv_enn_cfg6() && toff(); }

void TMC2130Stepper::push() {
  GCONF(GCONF_register.sr);
  IHOLD_IRUN(IHOLD_IRUN_register.sr);
  TPOWERDOWN(TPOWERDOWN_register.sr);
  TPWMTHRS(TPWMTHRS_register.sr);
  TCOOLTHRS(TCOOLTHRS_register.sr);
  THIGH(THIGH_register.sr);
  XDIRECT(XDIRECT_register.sr);
  VDCMIN(VDCMIN_register.sr);
  CHOPCONF(CHOPCONF_register.sr);
  COOLCONF(COOLCONF_register.sr);
  DCCTRL(DCCTRL_register.sr);
  PWMCONF(PWMCONF_register.sr);
  ENCM_CTRL(ENCM_CTRL_register.sr);
}

///////////////////////////////////////////////////////////////////////////////////////
// R: IOIN
uint32_t  TMC2130Stepper::IOIN()    { return read(IOIN_t::address); }
bool TMC2130Stepper::step()         { IOIN_t r{0}; r.sr = IOIN(); return r.step; }
bool TMC2130Stepper::dir()          { IOIN_t r{0}; r.sr = IOIN(); return r.dir; }
bool TMC2130Stepper::dcen_cfg4()    { IOIN_t r{0}; r.sr = IOIN(); return r.dcen_cfg4; }
bool TMC2130Stepper::dcin_cfg5()    { IOIN_t r{0}; r.sr = IOIN(); return r.dcin_cfg5; }
bool TMC2130Stepper::drv_enn_cfg6() { IOIN_t r{0}; r.sr = IOIN(); return r.drv_enn_cfg6; }
bool TMC2130Stepper::dco()          { IOIN_t r{0}; r.sr = IOIN(); return r.dco; }
uint8_t TMC2130Stepper::version()   { IOIN_t r{0}; r.sr = IOIN(); return r.version; }
///////////////////////////////////////////////////////////////////////////////////////
// W: TCOOLTHRS
uint32_t TMC2130Stepper::TCOOLTHRS() { return TCOOLTHRS_register.sr; }
void TMC2130Stepper::TCOOLTHRS(uint32_t input) {
  TCOOLTHRS_register.sr = input;
  write(TCOOLTHRS_register.address, TCOOLTHRS_register.sr);
}
///////////////////////////////////////////////////////////////////////////////////////
// W: THIGH
uint32_t TMC2130Stepper::THIGH() { return THIGH_register.sr; }
void TMC2130Stepper::THIGH(uint32_t input) {
  THIGH_register.sr = input;
  write(THIGH_register.address, THIGH_register.sr);
}
///////////////////////////////////////////////////////////////////////////////////////
// RW: XDIRECT
uint32_t TMC2130Stepper::XDIRECT() {
  return read(XDIRECT_register.address);
}
void TMC2130Stepper::XDIRECT(uint32_t input) {
  XDIRECT_register.sr = input;
  write(XDIRECT_register.address, XDIRECT_register.sr);
}
void TMC2130Stepper::coil_A(int16_t B)  { XDIRECT_register.coil_A = B; write(XDIRECT_register.address, XDIRECT_register.sr); }
void TMC2130Stepper::coil_B(int16_t B)  { XDIRECT_register.coil_B = B; write(XDIRECT_register.address, XDIRECT_register.sr); }
int16_t TMC2130Stepper::coil_A()        { XDIRECT_t r{0}; r.sr = XDIRECT(); return r.coil_A; }
int16_t TMC2130Stepper::coil_B()        { XDIRECT_t r{0}; r.sr = XDIRECT(); return r.coil_B; }
///////////////////////////////////////////////////////////////////////////////////////
// W: VDCMIN
uint32_t TMC2130Stepper::VDCMIN() { return VDCMIN_register.sr; }
void TMC2130Stepper::VDCMIN(uint32_t input) {
  VDCMIN_register.sr = input;
  write(VDCMIN_register.address, VDCMIN_register.sr);
}
///////////////////////////////////////////////////////////////////////////////////////
// RW: DCCTRL
void TMC2130Stepper::DCCTRL(uint32_t input) {
	DCCTRL_register.sr = input;
	write(DCCTRL_register.address, DCCTRL_register.sr);
}
void TMC2130Stepper::dc_time(uint16_t input) {
	DCCTRL_register.dc_time = input;
	write(DCCTRL_register.address, DCCTRL_register.sr);
}
void TMC2130Stepper::dc_sg(uint8_t input) {
	DCCTRL_register.dc_sg = input;
	write(DCCTRL_register.address, DCCTRL_register.sr);
}

uint32_t TMC2130Stepper::DCCTRL() {
	return read(DCCTRL_register.address);
}
uint16_t TMC2130Stepper::dc_time() {
	DCCTRL_t r{0};
  r.sr = DCCTRL();
	return r.dc_time;
}
uint8_t TMC2130Stepper::dc_sg() {
	DCCTRL_t r{0};
  r.sr = DCCTRL();
	return r.dc_sg;
}
///////////////////////////////////////////////////////////////////////////////////////
// R: PWM_SCALE
uint8_t TMC2130Stepper::PWM_SCALE() { return read(PWM_SCALE_t::address); }
///////////////////////////////////////////////////////////////////////////////////////
// W: ENCM_CTRL
uint8_t TMC2130Stepper::ENCM_CTRL() { return ENCM_CTRL_register.sr; }
void TMC2130Stepper::ENCM_CTRL(uint8_t input) {
  ENCM_CTRL_register.sr = input;
  write(ENCM_CTRL_register.address, ENCM_CTRL_register.sr);
}
void TMC2130Stepper::inv(bool B)      { ENCM_CTRL_register.inv = B;       write(ENCM_CTRL_register.address, ENCM_CTRL_register.sr); }
void TMC2130Stepper::maxspeed(bool B) { ENCM_CTRL_register.maxspeed  = B; write(ENCM_CTRL_register.address, ENCM_CTRL_register.sr); }
bool TMC2130Stepper::inv()            { return ENCM_CTRL_register.inv; }
bool TMC2130Stepper::maxspeed()       { return ENCM_CTRL_register.maxspeed; }
///////////////////////////////////////////////////////////////////////////////////////
// R: LOST_STEPS
uint32_t TMC2130Stepper::LOST_STEPS() { return read(LOST_STEPS_t::address); }

void TMC2130Stepper::sg_current_decrease(uint8_t value) {
  switch(value) {
    case 32: sedn(0b00); break;
    case  8: sedn(0b01); break;
    case  2: sedn(0b10); break;
    case  1: sedn(0b11); break;
  }
}
uint8_t TMC2130Stepper::sg_current_decrease() {
  switch(sedn()) {
    case 0b00: return 32;
    case 0b01: return  8;
    case 0b10: return  2;
    case 0b11: return  1;
  }
  return 0;
}