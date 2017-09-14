#include "cli.h"

static const char help[] =
  CLI_NAME ": Pololu Tic Command-line Utility\n"
  "Version " SOFTWARE_VERSION_STRING "\n"
  "Usage: " CLI_NAME " OPTIONS\n"
  "\n"
  "General options:\n"
  "  -s, --status                 Show device settings and info.\n"
  "  --full                       When used with --status, shows more.\n"
  "  -d SERIALNUMBER              Specifies the serial number of the device.\n"
  "  --list                       List devices connected to computer.\n"
  "  --pause                      Pause program at the end.\n"
  "  --pause-on-error             Pause program at the end if an error happens.\n"
  "  -h, --help                   Show this help screen.\n"
  "\n"
  "Control commands:\n"
  "  -p, --position NUM           Set target position in microsteps.\n"
  "  -y, --velocity NUM           Set target velocity in microsteps / 10000 s.\n"
  "  --halt-and-set-position NUM  Set where the controller thinks it currently is.\n"
  "  --halt-and-hold              Abruptly stop the motor.\n"
  "  --reset-command-timeout      Clears the command timeout error.\n"
  "  --deenergize                 Disable the motor driver.\n"
  "  --energize                   Stop disabling the driver.\n"
  "  --exit-safe-start            Send the exit safe start command.\n"
  "  --resume                     Equivalent to --energize with --exit-safe-start.\n"
  "  --enter-safe-start           Send the enter safe start command.\n"
  "  --reset                      Make the controller forget its current state.\n"
  "  --clear-driver-error         Attempt to clear a motor driver error.\n"
  "\n"
  "Temporary settings:\n"
  "  --max-speed NUM              Set the speed limit.\n"
  "  --starting-speed NUM         Set the starting speed.\n"
  "  --max-accel NUM              Set the acceleration limit.\n"
  "  --max-decel NUM              Set the deceleration limit.\n"
  "  --step-mode MODE             Set step mode: full, half, 1, 2, 4, 8, 16, 32.\n"
  "  --current NUM                Set the current limit in mA.\n"
  "  --decay MODE                 Set decay mode: mixed, slow, or fast.\n"
  "\n"
  "Permanent settings:\n"
  "  --restore-defaults           Restore device's factory settings\n"
  "  --settings FILE              Load settings file into device.\n"
  "  --get-settings FILE          Read device settings and write to file.\n"
  "  --fix-settings IN OUT        Read settings from a file and fix them.\n"
  "\n"
  "For more help, see: " DOCUMENTATION_URL "\n"
  "\n";

struct arguments
{
  bool show_status = false;

  bool full_output = false;

  bool serial_number_specified = false;
  std::string serial_number;

  bool show_list = false;

  bool pause = false;

  bool pause_on_error = false;

  bool show_help = false;

  bool set_target_position = false;
  int32_t target_position;

  bool set_target_velocity = false;
  int32_t target_velocity;

  bool halt_and_set_position = false;
  int32_t position;

  bool halt_and_hold = false;

  bool reset_command_timeout = false;

  bool deenergize = false;

  bool energize = false;

  bool exit_safe_start = false;

  bool enter_safe_start = false;

  bool reset = false;

  bool clear_driver_error = false;

  bool set_max_speed = false;
  uint32_t max_speed;

  bool set_starting_speed = false;
  uint32_t starting_speed;

  bool set_max_accel = false;
  uint32_t max_accel;

  bool set_max_decel = false;
  uint32_t max_decel;

  bool set_step_mode = false;
  uint8_t step_mode;

  bool set_current_limit = false;
  uint32_t current_limit;

  bool set_decay_mode = false;
  uint8_t decay_mode;

  bool restore_defaults = false;

  bool set_settings = false;
  std::string set_settings_filename;

  bool get_settings = false;
  std::string get_settings_filename;

  bool fix_settings = false;
  std::string fix_settings_input_filename;
  std::string fix_settings_output_filename;

  bool get_debug_data = false;

  uint32_t test_procedure = 0;

  bool action_specified() const
  {
    return show_status ||
      show_list ||
      show_help ||
      set_target_position ||
      set_target_velocity ||
      halt_and_set_position ||
      halt_and_hold ||
      reset_command_timeout ||
      deenergize ||
      energize ||
      exit_safe_start ||
      enter_safe_start ||
      reset ||
      clear_driver_error ||
      set_max_speed ||
      set_starting_speed ||
      set_max_accel ||
      set_max_decel ||
      set_step_mode ||
      set_current_limit ||
      set_decay_mode ||
      restore_defaults ||
      set_settings ||
      get_settings ||
      fix_settings ||
      get_debug_data ||
      test_procedure;
  }
};

// Note: This will not work correctly if T is uint64_t.
template <typename T>
static T parse_arg_int(arg_reader & arg_reader, int base = 10)
{
  const char * value_c = arg_reader.next();
  if (value_c == NULL)
  {
    throw exception_with_exit_code(EXIT_BAD_ARGS,
      "Expected a number after '" + std::string(arg_reader.last()) + "'.");
  }

  char * end;
  int64_t result = strtoll(value_c, &end, base);
  if (errno || *end)
  {
    throw exception_with_exit_code(EXIT_BAD_ARGS,
      "The number after '" + std::string(arg_reader.last()) + "' is invalid.");
  }

  if (result < std::numeric_limits<T>::min())
  {
    throw exception_with_exit_code(EXIT_BAD_ARGS,
      "The number after '" + std::string(arg_reader.last()) + "' is too small.");
  }

  if (result > std::numeric_limits<T>::max())
  {
    throw exception_with_exit_code(EXIT_BAD_ARGS,
      "The number after '" + std::string(arg_reader.last()) + "' is too large.");
  }

  return result;
}

static std::string parse_arg_string(arg_reader & arg_reader)
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
    return std::string(value_c);
}

static uint8_t parse_arg_step_mode(arg_reader & arg_reader)
{
  std::string mode_str = parse_arg_string(arg_reader);
  if (mode_str == "1" || mode_str == "full"
    || mode_str == "Full step" || mode_str == "full step")
  {
    return TIC_STEP_MODE_MICROSTEP1;
  }
  else if (mode_str == "2" || mode_str == "half" || mode_str == "1/2 step")
  {
    return TIC_STEP_MODE_MICROSTEP2;
  }
  else if (mode_str == "4" || mode_str == "1/4 step")
  {
    return TIC_STEP_MODE_MICROSTEP4;
  }
  else if (mode_str == "8" || mode_str == "1/8 step")
  {
    return TIC_STEP_MODE_MICROSTEP8;
  }
  else if (mode_str == "16" || mode_str == "1/16 step")
  {
    return TIC_STEP_MODE_MICROSTEP16;
  }
  else if (mode_str == "32" || mode_str == "1/32 step")
  {
    return TIC_STEP_MODE_MICROSTEP32;
  }
  else
  {
    throw exception_with_exit_code(EXIT_BAD_ARGS,
      "The step mode specified is invalid.");
  }
}

static uint8_t parse_arg_decay_mode(arg_reader & arg_reader)
{
  std::string decay_str = parse_arg_string(arg_reader);
  if (decay_str == "mixed" || decay_str == "Mixed")
  {
    return TIC_DECAY_MODE_MIXED;
  }
  else if (decay_str == "slow" || decay_str == "Slow")
  {
    return TIC_DECAY_MODE_SLOW;
  }
  else if (decay_str == "fast" || decay_str == "Fast")
  {
    return TIC_DECAY_MODE_FAST;
  }
  else if (decay_str == "mixed25" || decay_str == "Mixed 25%")
  {
    return TIC_DECAY_MODE_MIXED_25;
  }
  else if (decay_str == "mixed50" || decay_str == "Mixed 50%")
  {
    return TIC_DECAY_MODE_MIXED_25;
  }
  else if (decay_str == "mixed75" || decay_str == "Mixed 75%")
  {
    return TIC_DECAY_MODE_MIXED_25;
  }
  else
  {
    throw exception_with_exit_code(EXIT_BAD_ARGS,
      "The decay mode specified is invalid.");
  }
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
    else if (arg == "--full")
    {
      args.full_output = true;
    }
    else if (arg == "-d" || arg == "--serial")
    {
      args.serial_number_specified = true;
      args.serial_number = parse_arg_string(arg_reader);

      // Remove a pound sign at the beginning of the string because people might
      // copy that from the GUI.
      if (args.serial_number[0] == '#')
      {
        args.serial_number.erase(0, 1);
      }
    }
    else if (arg == "--list")
    {
      args.show_list = true;
    }
    else if (arg == "--pause")
    {
      args.pause = true;
    }
    else if (arg == "--pause-on-error")
    {
      args.pause_on_error = true;
    }
    else if (arg == "-h" || arg == "--help" ||
      arg == "--h" || arg == "-help" || arg == "/help" || arg == "/h")
    {
      args.show_help = true;
    }
    else if (arg == "-p" || arg == "--position")
    {
      args.set_target_position = true;
      args.target_position = parse_arg_int<int32_t>(arg_reader);
    }
    else if (arg == "-y" || arg == "--velocity")
    {
      args.set_target_velocity = true;
      args.target_velocity = parse_arg_int<int32_t>(arg_reader);
    }
    else if (arg == "--halt-and-set-position")
    {
      args.halt_and_set_position = true;
      args.position = parse_arg_int<int32_t>(arg_reader);
    }
    else if (arg == "--halt-and-hold")
    {
      args.halt_and_hold = true;
    }
    else if (arg == "--reset-command-timeout")
    {
      args.reset_command_timeout = true;
    }
    else if (arg == "--deenergize" || arg == "--de-energize")
    {
      args.deenergize = true;
    }
    else if (arg == "--energize")
    {
      args.energize = true;
    }
    else if (arg == "--exit-safe-start")
    {
      args.exit_safe_start = true;
    }
    else if (arg == "--resume")
    {
      args.energize = args.exit_safe_start = true;
    }
    else if (arg == "--enter-safe-start")
    {
      args.enter_safe_start = true;
    }
    else if (arg == "--reset")
    {
      args.reset = true;
    }
    else if (arg == "--clear-driver-error")
    {
      args.clear_driver_error = true;
    }
    else if (arg == "--max-speed")
    {
      args.set_max_speed = true;
      args.max_speed = parse_arg_int<uint32_t>(arg_reader);
    }
    else if (arg == "--starting-speed")
    {
      args.set_starting_speed = true;
      args.starting_speed = parse_arg_int<uint32_t>(arg_reader);
    }
    else if (arg == "--max-accel")
    {
      args.set_max_accel = true;
      args.max_accel = parse_arg_int<uint32_t>(arg_reader);
    }
    else if (arg == "--max-decel")
    {
      args.set_max_decel = true;
      args.max_decel = parse_arg_int<uint32_t>(arg_reader);
    }
    else if (arg == "--step-mode")
    {
      args.set_step_mode = true;
      args.step_mode = parse_arg_step_mode(arg_reader);
    }
    else if (arg == "--current" || arg == "--current-limit")
    {
      args.set_current_limit = true;
      args.current_limit = parse_arg_int<uint32_t>(arg_reader);
    }
    else if (arg == "--decay" || arg == "--decay-mode")
    {
      args.set_decay_mode = true;
      args.decay_mode = parse_arg_decay_mode(arg_reader);
    }
    else if (arg == "--restore-defaults" || arg == "--restoredefaults")
    {
      args.restore_defaults = true;
    }
    else if (arg == "--settings" || arg == "--set-settings" || arg == "--configure")
    {
      args.set_settings = true;
      args.set_settings_filename = parse_arg_string(arg_reader);
    }
    else if (arg == "--get-settings" || arg == "--getconf")
    {
      args.get_settings = true;
      args.get_settings_filename = parse_arg_string(arg_reader);
    }
    else if (arg == "--fix-settings")
    {
      args.fix_settings = true;
      args.fix_settings_input_filename = parse_arg_string(arg_reader);
      args.fix_settings_output_filename = parse_arg_string(arg_reader);
    }
    else if (arg == "--debug")
    {
      // This is an unadvertized option for helping customers troubleshoot
      // issues with their device.
      args.get_debug_data = true;
    }
    else if (arg == "--test")
    {
      // This option and the options below are unadvertised and helps us test
      // the software.
      args.test_procedure = parse_arg_int<uint32_t>(arg_reader);
    }
    else
    {
      throw exception_with_exit_code(EXIT_BAD_ARGS,
        std::string("Unknown option: '") + arg + "'.");
    }
  }
  return args;
}

static tic::handle handle(device_selector & selector)
{
  tic::device device = selector.select_device();
  return tic::handle(device);
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

static void set_current_limit_after_warning(device_selector & selector, uint32_t current_limit)
{
  tic::handle handle = ::handle(selector);

  uint32_t max_current = tic_get_max_allowed_current(handle.get_device().get_product());
  if (current_limit > max_current)
  {
    current_limit = max_current;
    std::cerr
      << "Warning: The current limit was too high "
      << "so it will be lowered to " << current_limit << " mA." << std::endl;
  }

  handle.set_current_limit(current_limit);
}

static void get_status(device_selector & selector, bool full_output)
{
  tic::device device = selector.select_device();
  tic::handle handle(device);
  tic::settings settings = handle.get_settings();
  tic::variables vars = handle.get_variables(true);
  std::string name = device.get_name();
  std::string serial_number = device.get_serial_number();
  std::string firmware_version = handle.get_firmware_version_string();
  print_status(vars, settings, name, serial_number, firmware_version, full_output);
}

static void restore_defaults(device_selector & selector)
{
  handle(selector).restore_defaults();
}

static void get_settings(device_selector & selector,
  const std::string & filename)
{
  tic::settings settings = handle(selector).get_settings();

  std::string warnings;
  settings.fix(&warnings);
  std::cerr << warnings;

  std::string settings_string = settings.to_string();

  write_string_to_file_or_pipe(filename, settings_string);
}

static void set_settings(device_selector & selector,
  const std::string & filename)
{
  std::string settings_string = read_string_from_file_or_pipe(filename);
  tic::settings settings = tic::settings::read_from_string(settings_string);

  std::string warnings;
  settings.fix(&warnings);
  std::cerr << warnings;

  tic::device device = selector.select_device();
  tic::handle handle(device);
  handle.set_settings(settings);
  handle.reinitialize();
}

static void fix_settings(const std::string & input_filename,
  const std::string & output_filename)
{
  std::string in_str = read_string_from_file_or_pipe(input_filename);
  tic::settings settings = tic::settings::read_from_string(in_str);

  std::string warnings;
  settings.fix(&warnings);
  std::cerr << warnings;

  write_string_to_file_or_pipe(output_filename, settings.to_string());
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

static void test_procedure(device_selector & selector, uint32_t procedure)
{
  if (procedure == 1)
  {
    // Let's print some fake variable data to test our print_status().  This
    // test invokes all sorts of undefined behavior but it's the easiest way to
    // put fake data into a tic::variables object without modifying
    // libpololu-tic.
    uint8_t fake_data[4096];
    memset(fake_data, 0xFF, sizeof(fake_data));
    tic::variables fake_vars((tic_variables *)fake_data);
    tic::settings settings;
    print_status(fake_vars, settings, "Fake name", "123", "9.99", true);
    fake_vars.pointer_release();
  }
  else if (procedure == 2)
  {
    tic::device device = selector.select_device();
    tic::handle handle(device);
    while (1)
    {
      tic::variables vars = handle.get_variables();
      std::cout << vars.get_analog_reading(TIC_PIN_NUM_SDA) << ','
                << vars.get_target_position() << ','
                << vars.get_acting_target_position() << ','
                << vars.get_current_position() << ','
                << vars.get_current_velocity() << ','
                << std::endl;
    }
  }
  else
  {
    throw std::runtime_error("Unknown test procedure.");
  }
}

// A note about ordering: We want to do all the setting stuff first because it
// could affect subsequent options.  We want to shoe the status last, because it
// could be affected by options before it.
static void run(const arguments & args)
{
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

  if (args.reset)
  {
    handle(selector).reset();
  }

  if (args.set_max_speed)
  {
    handle(selector).set_max_speed(args.max_speed);
  }

  if (args.set_starting_speed)
  {
    handle(selector).set_starting_speed(args.starting_speed);
  }

  if (args.set_max_accel)
  {
    handle(selector).set_max_accel(args.max_accel);
  }

  if (args.set_max_decel)
  {
    handle(selector).set_max_decel(args.max_decel);
  }

  if (args.halt_and_hold)
  {
    handle(selector).halt_and_hold();
  }

  if (args.reset_command_timeout)
  {
    handle(selector).reset_command_timeout();
  }

  if (args.energize)
  {
    handle(selector).energize();
  }

  // This should be after energize so that --resume does things in the same
  // order as the GUI.
  if (args.exit_safe_start)
  {
    handle(selector).exit_safe_start();
  }

  if (args.enter_safe_start)
  {
    handle(selector).enter_safe_start();
  }

  if (args.set_target_position)
  {
    handle(selector).set_target_position(args.target_position);
  }

  if (args.set_target_velocity)
  {
    handle(selector).set_target_velocity(args.target_velocity);
  }

  if (args.halt_and_set_position)
  {
    handle(selector).halt_and_set_position(args.position);
  }

  if (args.set_step_mode)
  {
    handle(selector).set_step_mode(args.step_mode);
  }

  if (args.set_current_limit)
  {
    set_current_limit_after_warning(selector, args.current_limit);
  }

  if (args.set_decay_mode)
  {
    handle(selector).set_decay_mode(args.decay_mode);
  }

  if (args.clear_driver_error)
  {
    handle(selector).clear_driver_error();
  }

  if (args.deenergize)
  {
    handle(selector).deenergize();
  }

  if (args.get_debug_data)
  {
    print_debug_data(selector);
  }

  if (args.test_procedure)
  {
    test_procedure(selector, args.test_procedure);
  }

  if (args.show_status)
  {
    get_status(selector, args.full_output);
  }
}

int main(int argc, char ** argv)
{
  int exit_code = 0;

  arguments args;
  try
  {
    args = parse_args(argc, argv);
    run(args);
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

  if (args.pause || (args.pause_on_error && exit_code))
  {
    std::cout << "Press enter to continue." << std::endl;
    char input;
    std::cin.get(input);
  }

  return exit_code;
}
