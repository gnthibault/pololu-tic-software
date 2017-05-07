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
  std::string input_settings_file_name;

  bool get_settings = false;
  std::string output_settings_file_name;

  bool show_help = false;

  bool get_debug_data = false;

  bool action_specified() const
  {
    return show_status ||
      show_list ||
      restore_defaults ||
      set_settings ||
      get_settings ||
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
      parse_arg_string(arg_reader, args.input_settings_file_name);
    }
    else if (arg == "--get-settings" || arg == "--getconf")
    {
      args.get_settings = true;
      parse_arg_string(arg_reader, args.output_settings_file_name);
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

static void print_status(device_selector & selector)
{
  tic::device device = selector.select_device();
  tic::handle handle(device);
  std::string firmware_version = handle.get_firmware_version_string();

  // The output here is compatible with YAML so that people can more easily
  // write scripts that use it.

  int left_column_width = 41;
  auto left_column = std::setw(left_column_width);

  std::cout << std::left << std::setfill(' ');

  std::cout << left_column << "Name: "
            << device.get_name() << std::endl;

  std::cout << left_column << "Serial number: "
            << device.get_serial_number() << std::endl;

  std::cout << left_column << "Firmware version: "
            << firmware_version << std::endl;

  std::cout << std::endl;

  // TODO: std::cout << "Current status:" << std::endl;
  // std::cout << leftColumn << "  Last device reset: "
  //          << Programmer::convertDeviceResetToString(variables.lastDeviceReset)
  //          << std::endl;
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
    // TODO: why is the "-d" option not working?
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

  if (args.get_settings)
  {
    get_settings(selector, args.output_settings_file_name);
  }

  if (args.restore_defaults)
  {
    restore_defaults(selector);
  }

  if (args.set_settings)
  {
    set_settings(selector, args.input_settings_file_name);
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
