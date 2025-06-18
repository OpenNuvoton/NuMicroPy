#!/usr/bin/env python
"""Creates the pin file for the M5531."""

from __future__ import print_function

import argparse
import sys
import csv

SUPPORTED_FN = {
    'OPA'   : ['P', 'N', 'O'],
    'ACMP'  : ['WLAT', 'P0', 'P1', 'P2', 'P3', 'N', 'O'],
    'BPWM'  : ['CH0', 'CH1', 'CH2', 'CH3', 'CH4', 'CH5'],
    'CAN'   : ['RXD', 'TXD'],
    'CANFD' : ['RXD', 'TXD'],
    'CCAP'  : ['HSYNC', 'VSYNC', 'SCLK', 'PIXCLK', 'SFIELD', 'DATA0', 'DATA1', 'DATA2', 'DATA3', 'DATA4', 'DATA5', 'DATA6', 'DATA7'],
    'DAC'   : ['ST', 'OUT'],
    'DMIC'  : ['CH0_CLK', 'CH0_DAT', 'CH0_CLKLP', 'CH1_CLK', 'CH1_DAT'],
    'EADC'  : ['CH0', 'CH1', 'CH2', 'CH3', 'CH4', 'CH5', 'CH6', 'CH7', 'CH8', 'CH9', 'CH10', 'CH11', 'CH12', 'CH13', 'CH14', 'CH15', 'CH16', 'CH17', 'CH18', 'CH19', 'CH20', 'CH21', 'CH22', 'CH23', 'ST'],
    'EBI'   : ['ADR0', 'ADR1', 'ADR2', 'ADR3', 'ADR4', 'ADR5', 'ADR6', 'ADR7', 'ADR8', 'ADR9', 'ADR10', 'ADR11', 'ADR12', 'ADR13', 'ADR14', 'ADR15', 'ADR16', 'ADR17', 'ADR18', 'ADR19',
               'AD0', 'AD1', 'AD2', 'AD3', 'AD4', 'AD5', 'AD6', 'AD7', 'AD8', 'AD9', 'AD10', 'AD11', 'AD12', 'AD13', 'AD14', 'AD15',
               'ALE', 'MCLK', 'nWR', 'nRD', 'nWRH', 'nWRL', 'nCS0', 'nCS1', 'nCS2'],
    'ECAP'  : ['IC0', 'IC1', 'IC2'],
    'EMAC'  : ['RMII_REFCLK', 'RMII_RXERR', 'RMII_CRSDV', 'PPS', 'RMII_RXD0', 'RMII_RXD1', 'RMII_TXEN', 'RMII_TXD0', 'RMII_TXD1', 'RMII_MDC', 'RMII_MDIO'],
    'EPWM'  : ['CH0', 'CH1', 'CH2', 'CH3', 'CH4', 'CH5', 'SYNC_OUT', 'SYNC_IN', 'BRAKE0', 'BRAKE1'],
    'EQEI'  : ['A', 'B', 'INDEX'],
    'ETM'   : ['TRACE_CLK', 'TRACE_DATA0', 'TRACE_DATA1', 'TRACE_DATA2', 'TRACE_DATA3'],
    'HSUSB' : ['VBUS_EN', 'VBUS_ST'],
    'I2C'   : ['SDA', 'SCL', 'SMBSUS', 'SMBAL'],
    'I2S'   : ['BCLK', 'MCLK', 'DI', 'DO', 'LRCK'],
    'I3C'   : ['SDA', 'SCL', 'PUPEN'],
    'INT'   : ['INT'],
    'KPI'   : ['COL0', 'COL1', 'COL2', 'COL3', 'COL4', 'COL5', 'COL6', 'COL7', 'ROW0', 'ROW1', 'ROW2', 'ROW3', 'ROW4', 'ROW5'],
    'LPADC' : ['CH0', 'CH1', 'CH2', 'CH3', 'CH4', 'CH5', 'CH6', 'CH7', 'CH8', 'CH9', 'CH10', 'CH11', 'CH12', 'CH13', 'CH14', 'CH15', 'CH16', 'CH17', 'CH18', 'CH19', 'CH20', 'CH21', 'CH22', 'CH23', 'ST'],
    'LPI2C' : ['SDA', 'SCL'],
    'LPSPI' : ['MOSI', 'MISO', 'CLK', 'SS'],
    'LPTM'  : ['LPTM', 'EXT'],
    'LPUART': ['RXD', 'nRTS', 'TXD', 'nCTS'],
    'PSIO'  : ['CH0', 'CH1', 'CH2', 'CH3', 'CH4', 'CH5', 'CH6', 'CH7'],
    'QEI'   : ['A', 'B', 'INDEX'],
    'QSPI'  : ['MOSI0', 'MISO0', 'MOSI1', 'MISO1', 'CLK', 'SS'],
    'SC'    : ['CLK', 'DAT', 'RST', 'PWR', 'nCD'],
    'SD'    : ['DAT0', 'DAT1', 'DAT2', 'DAT3', 'CLK', 'CMD', 'nCD'],
    'SPI'   : ['MOSI', 'MISO', 'CLK', 'SS', 'I2SMCLK'],
    'SPIM'  : ['MISO', 'MOSI', 'D2', 'D3', 'D4', 'D5', 'D6', 'D7', 'RWDS', 'CLK', 'CLKN', 'RESETN'],
    'TAMPER'  : ['TAMPER'],
    'TM'    : ['TM', 'EXT'],
    'UART'  : ['RXD', 'nRTS', 'TXD', 'nCTS'],
    'USB'   : ['VBUS', 'D_N', 'D_P', 'OTG_ID', 'VBUS_EN', 'VBUS_ST'],
    #'SWODEC': ['SWO'],
    #'USCI'  : ['CLK', 'CTL0', 'CTL1', 'DAT0', 'DAT1'],
}

CONDITIONAL_VAR = {
#TODO
#    'I2C'   : 'MICROPY_HW_I2C{num}_SCL',
#    'I2S'   : 'MICROPY_HW_ENABLE_I2S{num}',
#    'SPI'   : 'MICROPY_HW_SPI{num}_SCK',
#    'UART'  : 'MICROPY_HW_UART{num}_TX',
#    'USART' : 'MICROPY_HW_UART{num}_TX',
#    'SDMMC' : 'MICROPY_HW_SDMMC{num}_CK',
#    'CAN'   : 'MICROPY_HW_CAN{num}_TX',
}


def parse_port_pin(name_str):
    """Parses a string and returns a (port-num, pin-num) tuple."""
    if len(name_str) < 3:
        raise ValueError("Expecting pin name to be at least 3 charcters.")
    if name_str[0] != 'P':
        raise ValueError("Expecting pin name to start with P")
    if name_str[1] < 'A' or name_str[1] > 'K':
        raise ValueError("Expecting pin port to be between A and K")
    port = ord(name_str[1]) - ord('A')
    pin_str = name_str[2:]
    if not pin_str.isdigit():
        raise ValueError("Expecting numeric pin number.")
    return (port, int(pin_str))

def split_name_num(name_num):
    num = None
    for num_idx in range(len(name_num) - 1, -1, -1):
        if not name_num[num_idx].isdigit():
            name = name_num[0:num_idx + 1]
            num_str = name_num[num_idx + 1:]
            if len(num_str) > 0:
                num = int(num_str)
            break
    return name, num

def conditional_var(name_num):
    # Try the specific instance first. For example, if name_num is UART4_RX
    # then try UART4 first, and then try UART second.
    name, num = split_name_num(name_num)
    var = []
    if name in CONDITIONAL_VAR:
        var.append(CONDITIONAL_VAR[name].format(num=num))
    if name_num in CONDITIONAL_VAR:
        var.append(CONDITIONAL_VAR[name_num])
    return var

def print_conditional_if(cond_var, file=None):
    if cond_var:
        cond_str = []
        for var in cond_var:
            if var.find('ENABLE') >= 0:
                cond_str.append('(defined({0}) && {0})'.format(var))
            else:
                cond_str.append('defined({0})'.format(var))
        print('#if ' + ' || '.join(cond_str), file=file)

def print_conditional_endif(cond_var, file=None):
    if cond_var:
        print('#endif', file=file)

class AlternateFunction(object):
    """Holds the information associated with a pins alternate function."""

    def __init__(self, idx, af_str, port, pin):
        self.idx = idx
        # Special case. We change I2S2ext_SD into I2S2_EXTSD so that it parses
        # the same way the other peripherals do.
        af_str = af_str.replace('ext_', '_EXT')

        self.af_str = af_str

        self.func = ''
        self.fn_num = None
        self.pin_type = ''
        self.supported = False
        af_words = af_str.split('_', 1)
        #print(af_words)
        self.func, self.fn_num = split_name_num(af_words[0])

        self.port = port
        self.pin = pin

        if len(af_words) > 1:
            self.pin_type = af_words[1]
        #print(self.func)
        #print(self.fn_num)
        #print(self.pin_type)
        if self.func in SUPPORTED_FN:
            pin_types = SUPPORTED_FN[self.func]
            if self.pin_type in pin_types:
                self.supported = True
        #print(self.supported)

    def is_supported(self):
        return self.supported

    def ptr(self):
        """Returns the numbered function (i.e. USART6) for this AF."""
        if self.fn_num is None:
            return self.func
        return '{:s}{:d}'.format(self.func, self.fn_num)

    def mux_name(self):
        fn_num = self.fn_num
        if fn_num is None:
            fn_num = 0
        return 'AF_P{:s}{:d}_{:s}{:d}_{:s}'.format(self.port, self.pin, self.func, fn_num, self.pin_type)
#        return 'AF{:d}_{:s}'.format(self.idx, self.ptr())

    def mux_name_index(self):
        fn_num = self.fn_num
        if fn_num is None:
            fn_num = 0
        return 'AF_P{:s}{:d}_{:s}{:d}_{:s}:{:d}'.format(self.port, self.pin, self.func, fn_num, self.pin_type, self.idx)

    def print(self, mfp):
        """Prints the C representation of this AF."""
        cond_var = None
        if self.supported:
            cond_var = conditional_var('{}{}'.format(self.func, self.fn_num))
            print_conditional_if(cond_var)
            print('  AF',  end='')
        else:
            print('  //', end='')
        fn_num = self.fn_num
        if fn_num is None:
            fn_num = 0
        print('({:s}, {:d}, {:s}, {:2d}, {:8s}, {:2d}, {:10s}, {:8s}), // {:s}'.format(self.port, self.pin, mfp, self.idx,
              self.func, fn_num, self.pin_type, self.ptr(), self.af_str))
        print_conditional_endif(cond_var)

    def qstr_list(self):
        return [self.mux_name()]

class Pin(object):
    """Holds the information associated with a pin."""

    def __init__(self, port, pin, mfp):
        self.port = port
        self.pin = pin
        self.alt_fn = []
        self.alt_fn_count = 0
        self.adc_num = 0
        self.adc_channel = 0
        self.board_pin = False
        self.mfp = mfp

    def port_letter(self):
        return chr(self.port + ord('A'))

    def cpu_pin_name(self):
        return '{:s}{:d}'.format(self.port_letter(), self.pin)

    def is_board_pin(self):
        return self.board_pin

    def set_is_board_pin(self):
        self.board_pin = True

    def parse_af(self, af_idx, af_strs_in):
        if len(af_strs_in) == 0:
            return
        # If there is a slash, then the slash separates 2 aliases for the
        # same alternate function.
        af_strs = af_strs_in.split('/')
        for af_str in af_strs:
            alt_fn = AlternateFunction(af_idx, af_str, self.port_letter(), self.pin)
            self.alt_fn.append(alt_fn)
            if alt_fn.is_supported():
                self.alt_fn_count += 1

    def alt_fn_name(self, null_if_0=False):
        if null_if_0 and self.alt_fn_count == 0:
            return 'NULL'
        return 'pin_{:s}_af'.format(self.cpu_pin_name())

    def adc_num_str(self):
        str = ''
        for adc_num in range(1,4):
            if self.adc_num & (1 << (adc_num - 1)):
                if len(str) > 0:
                    str += ' | '
                str += 'PIN_ADC'
                str += chr(ord('0') + adc_num)
        if len(str) == 0:
            str = '0'
        return str


    def print(self):
        if self.alt_fn_count == 0:
            print("// ",  end='')
        print('const pin_af_obj_t {:s}[] = {{'.format(self.alt_fn_name()))
        for alt_fn in self.alt_fn:
            alt_fn.print(self.mfp)
        if self.alt_fn_count == 0:
            print("// ",  end='')
        print('};')
        print('')
        print('const pin_obj_t pin_{:s}_obj = PIN({:s}, {:d}, {:s}, {:s}, {:s}, {:d});'.format(
            self.cpu_pin_name(), self.port_letter(), self.pin, self.mfp,
            self.alt_fn_name(null_if_0=True),
            self.adc_num_str(), self.adc_channel))
        print('')

    def print_header(self, hdr_file):
        n = self.cpu_pin_name()
        hdr_file.write('extern const pin_obj_t pin_{:s}_obj;\n'.format(n))
        hdr_file.write('#define pin_{:s} (&pin_{:s}_obj)\n'.format(n, n))
        if self.alt_fn_count > 0:
            hdr_file.write('extern const pin_af_obj_t pin_{:s}_af[];\n'.format(n))

    def qstr_list(self):
        result = []
        for alt_fn in self.alt_fn:
            if alt_fn.is_supported():
                result += alt_fn.qstr_list()
        return result


class NamedPin(object):

    def __init__(self, name, pin):
        self._name = name
        self._pin = pin

    def pin(self):
        return self._pin

    def name(self):
        return self._name


class Pins(object):

    def __init__(self):
        self.cpu_pins = []   # list of NamedPin objects
        self.board_pins = [] # list of NamedPin objects

    def find_pin(self, port_num, pin_num):
        for named_pin in self.cpu_pins:
            pin = named_pin.pin()
            if pin.port == port_num and pin.pin == pin_num:
                return pin

    def parse_af_file(self, filename, pinname_col, mfp_col, af_col):
        with open(filename, 'r') as csvfile:
            rows = csv.reader(csvfile)
            for row in rows:
                try:
                    (port_num, pin_num) = parse_port_pin(row[pinname_col])
                except:
                    continue
                pin = Pin(port_num, pin_num, row[mfp_col])
                for af_idx in range(af_col, len(row)):
                    if af_idx < af_col + 32:
                        pin.parse_af(af_idx - af_col, row[af_idx])
                    elif af_idx == af_col + 32:
                        pin.parse_adc(row[af_idx])
                self.cpu_pins.append(NamedPin(pin.cpu_pin_name(), pin))

    def parse_board_file(self, filename):
        with open(filename, 'r') as csvfile:
            rows = csv.reader(csvfile)
            for row in rows:
                try:
                    (port_num, pin_num) = parse_port_pin(row[1])
                except:
                    continue
                pin = self.find_pin(port_num, pin_num)
                if pin:
                    pin.set_is_board_pin()
                    self.board_pins.append(NamedPin(row[0], pin))


    def print_named(self, label, named_pins):
        print('static const mp_rom_map_elem_t pin_{:s}_pins_locals_dict_table[] = {{'.format(label))
        for named_pin in named_pins:
            pin = named_pin.pin()
            if pin.is_board_pin():
                print('  {{ MP_ROM_QSTR(MP_QSTR_{:s}), MP_ROM_PTR(&pin_{:s}_obj) }},'.format(named_pin.name(),  pin.cpu_pin_name()))
        print('};')
        print('MP_DEFINE_CONST_DICT(pin_{:s}_pins_locals_dict, pin_{:s}_pins_locals_dict_table);'.format(label, label));

    def print(self):
        for named_pin in self.cpu_pins:
            pin = named_pin.pin()
            if pin.is_board_pin():
                pin.print()
        self.print_named('cpu', self.cpu_pins)
        print('')
        self.print_named('board', self.board_pins)

    def print_header(self, hdr_filename):
        with open(hdr_filename, 'wt') as hdr_file:
            for named_pin in self.cpu_pins:
                pin = named_pin.pin()
                if pin.is_board_pin():
                    pin.print_header(hdr_file)
            hdr_file.write('extern const pin_obj_t * const pin_adc1[];\n')
            hdr_file.write('extern const pin_obj_t * const pin_adc2[];\n')
            hdr_file.write('extern const pin_obj_t * const pin_adc3[];\n')
            # provide #define's mapping board to cpu name
            for named_pin in self.board_pins:
                hdr_file.write("#define pyb_pin_{:s} pin_{:s}\n".format(named_pin.name(), named_pin.pin().cpu_pin_name()))

    def print_qstr(self, qstr_filename):
        with open(qstr_filename, 'wt') as qstr_file:
            qstr_set = set([])
            for named_pin in self.cpu_pins:
                pin = named_pin.pin()
                if pin.is_board_pin():
                    qstr_set |= set(pin.qstr_list())
                    qstr_set |= set([named_pin.name()])
            for named_pin in self.board_pins:
                qstr_set |= set([named_pin.name()])
            for qstr in sorted(qstr_set):
                cond_var = None
                if qstr.startswith('AF'):
                    af_words = qstr.split('_')
                    cond_var = conditional_var(af_words[1])
                    print_conditional_if(cond_var, file=qstr_file)
                print('Q({})'.format(qstr), file=qstr_file)
                print_conditional_endif(cond_var, file=qstr_file)

    def print_af_hdr(self, af_const_filename):
        with open(af_const_filename,  'wt') as af_const_file:
            af_hdr_set = set([])
            mux_name_width = 0
            for named_pin in self.cpu_pins:
                pin = named_pin.pin()
                if pin.is_board_pin():
                    for af in pin.alt_fn:
                        if af.is_supported():
                            mux_name = af.mux_name_index()
                            af_hdr_set |= set([mux_name])
                            if len(mux_name) > mux_name_width:
                                mux_name_width = len(mux_name)
            for mux_name in sorted(af_hdr_set):
                af_words = mux_name.split('_')  # ex mux_name: AF_UART5_TXD:2
                af_name_index = mux_name.split(':') 
                cond_var = conditional_var(af_words[2])
                print_conditional_if(cond_var, file=af_const_file)
                key = 'MP_ROM_QSTR(MP_QSTR_{}),'.format(af_name_index[0])
#                val = 'MP_ROM_INT(GPIO_{})'.format(mux_name)
                val = 'MP_ROM_INT({:s})'.format(af_name_index[1])
                print('    { %-*s %s },' % (mux_name_width + 26, key, val),
                      file=af_const_file)
                print_conditional_endif(cond_var, file=af_const_file)

def main():
    parser = argparse.ArgumentParser(
        prog="make-pins.py",
        usage="%(prog)s [options] [command]",
        description="Generate board specific pin file"
    )
    parser.add_argument(
        "-a", "--af",
        dest="af_filename",
        help="Specifies the alternate function file for the chip",
        default="m26x_af.csv"
    )
    parser.add_argument(
        "--af-const",
        dest="af_const_filename",
        help="Specifies header file for alternate function constants.",
        default="build/pins_af_const.h"
    )
    parser.add_argument(
        "--af-py",
        dest="af_py_filename",
        help="Specifies the filename for the python alternate function mappings.",
        default="build/pins_af.py"
    )
    parser.add_argument(
        "-b", "--board",
        dest="board_filename",
        help="Specifies the board file",
    )
    parser.add_argument(
        "-p", "--prefix",
        dest="prefix_filename",
        help="Specifies beginning portion of generated pins file",
        default="m26x_prefix.c"
    )
    parser.add_argument(
        "-q", "--qstr",
        dest="qstr_filename",
        help="Specifies name of generated qstr header file",
        default="build/pins_qstr.h"
    )
    parser.add_argument(
        "-r", "--hdr",
        dest="hdr_filename",
        help="Specifies name of generated pin header file",
        default="build/pins.h"
    )
    args = parser.parse_args(sys.argv[1:])

    pins = Pins()

    print('// This file was automatically generated by make-pins.py')
    print('//')
    if args.af_filename:
        print('// --af {:s}'.format(args.af_filename))
        pins.parse_af_file(args.af_filename, 1, 2, 3)

    if args.prefix_filename:
        print('// --prefix {:s}'.format(args.prefix_filename))
        print('')
        with open(args.prefix_filename, 'r') as prefix_file:
            print(prefix_file.read())

    if args.board_filename:
        print('// --board {:s}'.format(args.board_filename))
        pins.parse_board_file(args.board_filename)


    pins.print()
    pins.print_header(args.hdr_filename)
    pins.print_qstr(args.qstr_filename)
    pins.print_af_hdr(args.af_const_filename)

if __name__ == "__main__":
    main()

