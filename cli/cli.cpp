#include <tic.hpp>
#include "config.h"
#include "arg_reader.h"
#include "device_selector.h"
#include "exit_codes.h"
#include "exception_with_exit_code.h"
#include "file_utils.h"

#include <bitset>
#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

// [all-settings]
static const char help[] =
  CLI_NAME ": Pololu Tic USB Stepper Motor Controller Command-line Utility\n"
  "Version " SOFTWARE_VERSION_STRING "\n"
  "Usage: " CLI_NAME " OPTIONS\n"
  "\n"
  "General options:\n"
  "  -s, --status                Show device settings and info.\n"
  "  -d SERIALNUMBER             Specifies the serial number of the device.\n"
  "  --list                      List devices connected to computer.\n"
  "  --restore-defaults          Restore device's factory settings\n"
  "  --settings FILE             Load settings file into device.\n"
  "  --get-settings FILE         Read device settings and write to file.\n"
  "  --fix-settings IN OUT       Read settings from a file and fix them.\n"
  "  -h, --help                  Show this help screen.\n"
  "\n"
  "For more help, see: " DOCUMENTATION_URL "\n"
  "\n";

struct arguments
{
  bool show_status = false;

  bool serial_number_specified = false;
  std::string serial_number;

  bool show_list = false;

  bool restore_defaults = false;

  bool set_settings = false;
  std::string set_settings_filename;

  bool get_settings = false;
  std::string get_settings_filename;

  bool fix_settings = false;
  std::string fix_settings_input_filename;
  std::string fix_settings_output_filename;

  bool show_help = false;

  bool get_debug_data = false;

  bool action_specified() const
  {
    return show_status ||
      show_list ||
      restore_defaults ||
      set_settings ||
      get_settings ||
      fix_settings ||
      show_help ||
      get_debug_data;
  }
};

static void parse_arg_serial_number(arg_reader & arg_reader, arguments & args)
{
  const char * value_c = arg_reader.next();
  if (value_c == NULL)
  {
    throw exception_with_exit_code(EXIT_BAD_ARGS,
      "Expected a serial number after '"
      + std::string(arg_reader.last()) + "'.");
  }
  if (value_c[0] == 0)
  {
    throw exception_with_exit_code(EXIT_BAD_ARGS,
      "An empty serial number was specified.");
  }

  args.serial_number_specified = true;
  args.serial_number = value_c;
}

static void parse_arg_string(arg_reader & arg_reader, std::string & str)
{
    const char * value_c = arg_reader.next();
    if (value_c == NULL)
    {
      throw exception_with_exit_code(EXIT_BAD_ARGS,
        "Expected an argument after '" +
        std::string(arg_reader.last()) + "'.");
    }
    if (value_c[0] == 0)
    {
      throw exception_with_exit_code(EXIT_BAD_ARGS,
        "Expected a non-empty argument after '" +
        std::string(arg_reader.last()) + "'.");
    }
    str = value_c;
}

static arguments parse_args(int argc, char ** argv)
{
  arg_reader arg_reader(argc, argv);
  arguments args;

  while (1)
  {
    const char * arg_c = arg_reader.next();
    if (arg_c == NULL)
    {
      break;  // Done reading arguments.
    }

    std::string arg = arg_c;

    if (arg == "-s" || arg == "--status")
    {
      args.show_status = true;
    }
    else if (arg == "-d" || arg == "--serial")
    {
      parse_arg_serial_number(arg_reader, args);
    }
    else if (arg == "--restore-defaults" || arg == "--restoredefaults")
    {
      args.restore_defaults = true;
    }
    else if (arg == "--settings" || arg == "--set-settings" || arg == "--configure")
    {
      args.set_settings = true;
      parse_arg_string(arg_reader, args.set_settings_filename);
    }
    else if (arg == "--get-settings" || arg == "--getconf")
    {
      args.get_settings = true;
      parse_arg_string(arg_reader, args.get_settings_filename);
    }
    else if (arg == "--fix-settings")
    {
      args.fix_settings = true;
      parse_arg_string(arg_reader, args.fix_settings_input_filename);
      parse_arg_string(arg_reader, args.fix_settings_output_filename);
    }
    else if (arg == "--list")
    {
      args.show_list = true;
    }
    else if (arg == "-h" || arg == "--help" ||
      arg == "--h" || arg == "-help" || arg == "/help" || arg == "/h")
    {
      args.show_help = true;
    }
    else if (arg == "--debug")
    {
      args.get_debug_data = true;
    }
    else
    {
      throw exception_with_exit_code(EXIT_BAD_ARGS,
        std::string("Unknown option: '") + arg + "'.");
    }
  }
  return args;
}

static void print_list(device_selector & selector)
{
  for (const tic::device & instance : selector.list_devices())
  {
    std::cout << std::left << std::setfill(' ');
    std::cout << std::setw(17) << instance.get_serial_number() + "," << " ";
    std::cout << std::setw(45) << instance.get_name();
    std::cout << std::endl;
  }
}

static void print_errors(uint32_t errors, const char * error_set_name)
{
  std::cout << error_set_name << ":" << std::endl;
  for (uint32_t i = 0; i < 32; i++)
  {
    uint32_t error = (1 << i);
    if (errors & error)
    {
      std::cout << tic_look_up_error_string(error) << std::endl;
    }
  }
}

static void print_status(device_selector & selector)
{
  tic::device device = selector.select_device();
  tic::handle handle(device);
  std::string firmware_version = handle.get_firmware_version_string();
  bool clear_events = true;
  tic::variables vars = handle.get_variables(clear_events);

  int left_column_width = 41;
  auto left_column = std::setw(left_column_width);

  // The output here is YAML so that people can more easily write scripts that
  // use it.

  std::cout << std::left << std::setfill(' ');

  std::cout << left_column << "Model: "
            << device.get_name() << std::endl;

  std::cout << left_column << "Serial number: "
            << device.get_serial_number() << std::endl;

  std::cout << left_column << "Firmware version: "
            << firmware_version << std::endl;

  std::cout << left_column << "Last reset: "
            << tic_look_up_device_reset_string(vars.get_device_reset())
            << std::endl;

  std::cout << std::endl;

  std::cout << left_column << "Operation state:"
            << tic_look_up_operation_state_string(vars.get_operation_state())
            << std::endl;

  print_errors(vars.get_error_status(),
    "Errors currently stopping the motor");
  print_errors(vars.get_error_occurred(),
    "Errors that occured since last check");

  std::cout << std::endl;

  std::cout << left_column << "Planning mode:"
            << tic_look_up_planning_mode_string(vars.get_planning_mode())
            << std::endl;

  std::cout << left_column << "Target position:"
            << vars.get_target_position() << std::endl;

  std::cout << left_column << "Target velocity:"
            << vars.get_target_velocity() << std::endl;

  std::cout << left_column << "Speed min:"
            << vars.get_speed_min() << std::endl;

  std::cout << left_column << "Speed max:"
            << vars.get_speed_max() << std::endl;

  std::cout << left_column << "Decel max:"
            << vars.get_decel_max() << std::endl;

  std::cout << left_column << "Accel max:"
            << vars.get_accel_max() << std::endl;

  std::cout << left_column << "Current position:"
            << vars.get_current_position() << std::endl;

  std::cout << left_column << "Current velocity:"
            << vars.get_current_velocity() << std::endl;

  std::cout << left_column << "Acting target position:"
            << vars.get_acting_target_position() << std::endl;

  std::cout << left_column << "Time since last step:"
            << vars.get_time_since_last_step() << std::endl;

  std::cout << std::endl;

  std::cout << left_column << "VIN: "
            << vars.get_vin_voltage() << " mV"
            << std::endl;

  std::cout << left_column << "Up time: "
            << vars.get_up_time() << " ms"  // TODO: HH:MM:SS:mmm format
            << std::endl;

  std::cout << left_column << "Encoder position: "
            << vars.get_encoder_position()
            << std::endl;

  std::cout << left_column << "RC pulse width: "
            << vars.get_rc_pulse_width()
            << std::endl;

  std::cout << left_column << "Analog reading on SCL: "
            << vars.get_analog_reading(TIC_PIN_NUM_SCL)
            << std::endl;

  std::cout << left_column << "Analog reading on SDA: "
            << vars.get_analog_reading(TIC_PIN_NUM_SDA)
            << std::endl;

  std::cout << left_column << "Analog reading on TX: "
            << vars.get_analog_reading(TIC_PIN_NUM_TX)
            << std::endl;

  std::cout << left_column << "Analog reading on RX: "
            << vars.get_analog_reading(TIC_PIN_NUM_RX)
            << std::endl;

  std::cout << left_column << "Digital reading on SCL:"
            << vars.get_digital_reading(TIC_PIN_NUM_SCL)
            << std::endl;

  std::cout << left_column << "Digital reading on SDA:"
            << vars.get_digital_reading(TIC_PIN_NUM_SDA)
            << std::endl;

  std::cout << left_column << "Digital reading on TX:"
            << vars.get_digital_reading(TIC_PIN_NUM_TX)
            << std::endl;

  std::cout << left_column << "Digital reading on RX:"
            << vars.get_digital_reading(TIC_PIN_NUM_RX)
            << std::endl;

  std::cout << left_column << "Digital reading on RC:"
            << vars.get_digital_reading(TIC_PIN_NUM_RC)
            << std::endl;

  std::cout << left_column << "Switch on SCL:"
            << vars.get_switch_status(TIC_PIN_NUM_SCL)
            << std::endl;

  std::cout << left_column << "Switch on SDA:"
            << vars.get_switch_status(TIC_PIN_NUM_SDA)
            << std::endl;

  std::cout << left_column << "Switch on TX:"
            << vars.get_switch_status(TIC_PIN_NUM_TX)
            << std::endl;

  std::cout << left_column << "Switch on RX:"
            << vars.get_switch_status(TIC_PIN_NUM_RX)
            << std::endl;

  std::cout << left_column << "Switch on RC:"
            << vars.get_switch_status(TIC_PIN_NUM_RC)
            << std::endl;

  std::cout << left_column << "Pin state for SCL:"
            << tic_look_up_pin_state_string(vars.get_pin_state(TIC_PIN_NUM_SCL))
            << std::endl;

  std::cout << left_column << "Pin state for SDA:"
            << tic_look_up_pin_state_string(vars.get_pin_state(TIC_PIN_NUM_SDA))
            << std::endl;

  std::cout << left_column << "Pin state for TX:"
            << tic_look_up_pin_state_string(vars.get_pin_state(TIC_PIN_NUM_TX))
            << std::endl;

  std::cout << left_column << "Pin state for RX:"
            << tic_look_up_pin_state_string(vars.get_pin_state(TIC_PIN_NUM_RX))
            << std::endl;

  // Even though it cannot actually be set, let's display it for the curious.
  std::cout << left_column << "Pin state for RC:"
            << tic_look_up_pin_state_string(vars.get_pin_state(TIC_PIN_NUM_RC))
            << std::endl;

  std::cout << left_column << "Step mode:"
            << tic_look_up_step_mode_string(vars.get_step_mode())
            << std::endl;

  std::cout << left_column << "Decay mode:"
            << tic_look_up_decay_mode_string(vars.get_decay_mode())
            << std::endl;

  std::cout << std::endl;
}

static void restore_defaults(device_selector & selector)
{
  tic::device device = selector.select_device();
  tic::handle(device).restore_defaults();

  // Give the Tic time to modify its settings.
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
}

static void get_settings(device_selector & selector,
  const std::string & filename)
{
  tic::device device = selector.select_device();
  tic::settings settings = tic::handle(device).get_settings();
  std::string settings_string = settings.to_string();

  std::string warnings;
  settings.fix(&warnings);
  std::cerr << warnings;

  write_string_to_file(filename, settings_string);
}

static void set_settings(device_selector & selector,
  const std::string & filename)
{
  std::string settings_string = read_string_from_file(filename);

  std::string warnings;
  tic::settings settings = tic::settings::read_from_string(
    settings_string, &warnings);
  std::cerr << warnings;

  settings.fix(&warnings);
  std::cerr << warnings;

  tic::device device = selector.select_device();
  tic::handle(device).set_settings(settings);
}

static void fix_settings(const std::string & input_filename,
  const std::string & output_filename)
{
  std::string in_str = read_string_from_file(input_filename);

  std::string warnings;
  tic::settings settings = tic::settings::read_from_string(in_str, &warnings);
  std::cerr << warnings;

  settings.fix(&warnings);
  std::cerr << warnings;

  write_string_to_file(output_filename, settings.to_string());
}

static void print_debug_data(device_selector & selector)
{
  tic::device device = selector.select_device();
  tic::handle handle(device);

  std::vector<uint8_t> data(4096, 0);
  handle.get_debug_data(data);

  for (const uint8_t & byte : data)
  {
    std::cout << std::setfill('0') << std::setw(2) << std::hex
              << (unsigned int)byte << ' ';
  }
  std::cout << std::endl;
}

static void run(int argc, char ** argv)
{
  arguments args = parse_args(argc, argv);

  if (args.show_help || !args.action_specified())
  {
    std::cout << help;
    return;
  }

  device_selector selector;
  if (args.serial_number_specified)
  {
    selector.specify_serial_number(args.serial_number);
  }

  if (args.show_list)
  {
    print_list(selector);
    return;
  }

  if (args.show_status)
  {
    print_status(selector);
  }

  if (args.fix_settings)
  {
    fix_settings(args.fix_settings_input_filename,
      args.fix_settings_output_filename);
  }

  if (args.get_settings)
  {
    get_settings(selector, args.get_settings_filename);
  }

  if (args.restore_defaults)
  {
    restore_defaults(selector);
  }

  if (args.set_settings)
  {
    set_settings(selector, args.set_settings_filename);
  }

  if (args.get_debug_data)
  {
    print_debug_data(selector);
  }
}

int main(int argc, char ** argv)
{
  int exit_code = 0;

  try
  {
    run(argc, argv);
  }
  catch (const exception_with_exit_code & error)
  {
    std::cerr << "Error: " << error.what() << std::endl;
    exit_code = error.get_code();
  }
  catch (const std::exception & error)
  {
    std::cerr << "Error: " << error.what() << std::endl;
    exit_code = EXIT_OPERATION_FAILED;
  }

  return exit_code;
}
