#include "main_window.h"
#include "main_controller.h"
#include "config.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QShortcut>
#include <QSpinBox>
#include <QTabWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include <cassert>

main_window::main_window(QWidget *parent)
  : QMainWindow(parent)
{
  setup_window();
}

void main_window::set_controller(main_controller * controller)
{
  this->controller = controller;
}

void main_window::start_update_timer(uint32_t interval_ms)
{
  assert(interval_ms <= std::numeric_limits<int>::max());
  update_timer->start(interval_ms);
}

void main_window::show_error_message(std::string const & message)
{
  QMessageBox mbox(QMessageBox::Critical, windowTitle(),
    QString(message.c_str()));
  mbox.exec();
}

void main_window::show_warning_message(std::string const & message)
{
  QMessageBox mbox(QMessageBox::Warning, windowTitle(),
    QString(message.c_str()));
  mbox.exec();
}

void main_window::show_info_message(std::string const & message)
{
  QMessageBox mbox(QMessageBox::Information, windowTitle(),
    QString(message.c_str()));
  mbox.exec();
}

bool main_window::confirm(std::string const & question)
{
  QMessageBox mbox(QMessageBox::Question, windowTitle(),
    QString(question.c_str()), QMessageBox::Ok | QMessageBox::Cancel);
  int button = mbox.exec();
  return button == QMessageBox::Ok;
}

void main_window::set_device_list_contents(std::vector<tic::device> const & device_list)
{
  suppress_events = true;
  device_list_value->clear();
  device_list_value->addItem(tr("Not connected"), QString()); // null value
  for (tic::device const & device : device_list)
  {
    device_list_value->addItem(
      QString((device.get_short_name() + " #" +device.get_serial_number()).c_str()),
      QString(device.get_os_id().c_str()));
  }
  suppress_events = false;
}

void main_window::set_device_list_selected(tic::device const & device)
{
  // TODO: show an error if we couldn't find the specified device
  // (findData returned -1)?
  suppress_events = true;
  int index = 0;
  if (device)
  {
    index = device_list_value->findData(QString(device.get_os_id().c_str()));
  }
  device_list_value->setCurrentIndex(index);
  suppress_events = false;
}

void main_window::set_connection_status(std::string const & status, bool error)
{
  if (error)
  {
    connection_status_value->setStyleSheet("QLabel { color: red; }");
  }
  else
  {
    connection_status_value->setStyleSheet("");
  }
  connection_status_value->setText(QString(status.c_str()));
}

void main_window::set_tab_pages_enabled(bool enabled)
{
  status_page_widget->setEnabled(enabled);
  settings_page_widget->setEnabled(enabled);
}

void main_window::set_manual_target_box_enabled(bool enabled)
{
  manual_target_box->setEnabled(enabled);
}

void main_window::set_disable_driver_button_enabled(bool enabled)
{
  disable_driver_button->setEnabled(enabled);
}

void main_window::set_enable_driver_button_enabled(bool enabled)
{
  enable_driver_button->setEnabled(enabled);
}

void main_window::set_apply_settings_enabled(bool enabled)
{
  apply_settings_button->setEnabled(enabled);
  apply_settings_action->setEnabled(enabled);
}

void main_window::set_disconnect_enabled(bool enabled)
{
  disconnect_action->setEnabled(enabled);
}

void main_window::set_reload_settings_enabled(bool enabled)
{
  reload_settings_action->setEnabled(enabled);
}

void main_window::set_restore_defaults_enabled(bool enabled)
{
  restore_defaults_action->setEnabled(enabled);
}

void main_window::set_device_name(std::string const & name, bool link_enabled)
{
  QString text = QString(name.c_str());
  if (link_enabled)
  {
    text = "<a href=\"#doc\">" + text + "</a>";
  }
  device_name_value->setText(text);
}

void main_window::set_serial_number(std::string const & serial_number)
{
  serial_number_value->setText(QString(serial_number.c_str()));
}

void main_window::set_firmware_version(std::string const & firmware_version)
{
  firmware_version_value->setText(QString(firmware_version.c_str()));
}

void main_window::set_vin_voltage(std::string const & vin_voltage)
{
  vin_voltage_value->setText(QString(vin_voltage.c_str()));
}

void main_window::set_target_position(std::string const & target_position)
{
  target_label->setText(tr("Target position:"));
  target_value->setText(QString(target_position.c_str()));
}

void main_window::set_target_velocity(std::string const & target_velocity)
{
  target_label->setText(tr("Target velocity:"));
  target_value->setText(QString(target_velocity.c_str()));
}

void main_window::set_target_none()
{
  target_label->setText(tr("Target:"));
  target_value->setText(tr("No target"));
}

void main_window::set_current_position(std::string const & current_position)
{
  current_position_value->setText(QString(current_position.c_str()));
}

void main_window::set_current_velocity(std::string const & current_velocity)
{
  current_velocity_value->setText(QString(current_velocity.c_str()));
}

void main_window::set_error_status(uint16_t error_status)
{
  for (int i = 0; i < 16; i++)
  {
    if (error_rows[i].stopping_value == NULL) { continue; }

    if (error_status & (1 << i))
    {
      error_rows[i].stopping_value->setText(tr("Yes"));
      error_rows[i].stopping_value->setStyleSheet(
        "QLabel:enabled { background-color: red; color: white; }");
    }
    else
    {
      error_rows[i].stopping_value->setText(tr("No"));
      error_rows[i].stopping_value->setStyleSheet(styleSheet());
    }
  }
}

void main_window::increment_errors_occurred(uint32_t errors_occurred)
{
  for (int i = 0; i < 32; i++)
  {
    if (error_rows[i].count_value == NULL) { continue; }

    if (errors_occurred & (1 << i))
    {
      error_rows[i].count++;
      error_rows[i].count_value->setText(QString::number(error_rows[i].count));
    }
  }
}

void main_window::reset_error_counts()
{
  for (int i = 0; i < 32; i++)
  {
    if (error_rows[i].count_value == NULL) { continue; }

    error_rows[i].count = 0;
    error_rows[i].count_value->setText(tr("-"));
  }
}

void main_window::set_control_mode(uint8_t control_mode)
{
  set_u8_combo_box(control_mode_value, control_mode);
}

void main_window::set_manual_target_range(int32_t target_min, int32_t target_max)
{
  suppress_events = true;
  manual_target_scroll_bar->setMinimum(target_min);
  manual_target_scroll_bar->setMaximum(target_max);
  manual_target_scroll_bar->setPageStep((target_max - target_min) / 20);
  manual_target_min_label->setText(QString::number(target_min));
  manual_target_max_label->setText(QString::number(target_max));
  manual_target_entry_value->setRange(target_min, target_max);
  suppress_events = false;
}

void main_window::set_manual_target(int32_t target)
{
  suppress_events = true;
  manual_target_entry_value->setValue(target);
  manual_target_scroll_bar->setValue(target);
  suppress_events = false;
}

void main_window::set_serial_baud_rate(uint32_t serial_baud_rate)
{
  set_spin_box(serial_baud_rate_value, serial_baud_rate);
}

void main_window::set_serial_device_number(uint8_t serial_device_number)
{
  set_spin_box(serial_device_number_value, serial_device_number);
}

void main_window::set_serial_crc_enabled(bool serial_crc_enabled)
{
  set_check_box(serial_crc_enabled_check, serial_crc_enabled);
}

void main_window::set_input_min(uint32_t input_min)
{
  set_spin_box(input_min_value, input_min);
}

void main_window::set_input_neutral_min(uint32_t input_neutral_min)
{
  set_spin_box(input_neutral_min_value, input_neutral_min);
}

void main_window::set_input_neutral_max(uint32_t input_neutral_max)
{
  set_spin_box(input_neutral_max_value, input_neutral_max);
}

void main_window::set_input_max(uint32_t input_max)
{
  set_spin_box(input_max_value, input_max);
}

void main_window::set_output_min(int32_t output_min)
{
  set_spin_box(output_min_value, output_min);
}

void main_window::set_output_max(int32_t output_max)
{
  set_spin_box(output_max_value, output_max);
}

void main_window::set_input_averaging_enabled(bool input_averaging_enabled)
{
  set_check_box(input_averaging_enabled_check, input_averaging_enabled);
}

void main_window::set_input_hysteresis(uint16_t input_hysteresis)
{
  set_spin_box(input_hysteresis_value, input_hysteresis);
}

void main_window::set_encoder_prescaler(uint32_t encoder_prescaler)
{
  set_spin_box(encoder_prescaler_value, encoder_prescaler);
}

void main_window::set_encoder_postscaler(uint32_t encoder_postscaler)
{
  set_spin_box(encoder_postscaler_value, encoder_postscaler);
}

void main_window::set_encoder_unlimited(bool encoder_unlimited)
{
  set_check_box(encoder_unlimited_check, encoder_unlimited);
}

void main_window::set_speed_max(uint32_t speed_max)
{
  set_spin_box(speed_max_value, speed_max);
  speed_max_value_pretty->setText(QString(convert_speed_to_pps_string(speed_max).c_str()));
}

void main_window::set_speed_min(uint32_t speed_min)
{
  set_spin_box(speed_min_value, speed_min);
  speed_min_value_pretty->setText(QString(convert_speed_to_pps_string(speed_min).c_str()));
}

void main_window::set_accel_max(uint32_t accel_max)
{
  set_spin_box(accel_max_value, accel_max);
  accel_max_value_pretty->setText(QString(convert_accel_to_pps2_string(accel_max).c_str()));
}

void main_window::set_decel_max(uint32_t decel_max)
{
  set_spin_box(decel_max_value, decel_max);
  if (decel_max == 0)
  {
    decel_max_value_pretty->setText(tr("= acceleration max"));
  }
  else
  {
    decel_max_value_pretty->setText(QString(convert_accel_to_pps2_string(decel_max).c_str()));
  }
}

void main_window::set_step_mode(uint8_t step_mode)
{
  set_u8_combo_box(step_mode_value, step_mode);
}

void main_window::set_current_limit(uint32_t current_limit)
{
  set_spin_box(current_limit_value, current_limit);
}

void main_window::set_decay_mode(uint8_t decay_mode)
{
  set_u8_combo_box(decay_mode_value, decay_mode);
}

void main_window::set_disable_safe_start(bool disable_safe_start)
{
  set_check_box(disable_safe_start_check, disable_safe_start);
}

void main_window::set_ignore_err_line_high(bool ignore_err_line_high)
{
  set_check_box(ignore_err_line_high_check, ignore_err_line_high);
}

void main_window::set_u8_combo_box(QComboBox * combo, uint8_t value)
{
  suppress_events = true;
  combo->setCurrentIndex(combo->findData(value));
  suppress_events = false;
}

void main_window::set_spin_box(QSpinBox * spin, int value)
{
  // Only set the QSpinBox's value if the new value is numerically different.
  // This prevents, for example, a value of "0000" from being changed to "0"
  // while you're trying to change "10000" to "20000".
  if (spin->value() != value)
  {
    suppress_events = true;
    spin->setValue(value);
    suppress_events = false;
  }
}

void main_window::set_check_box(QCheckBox * check, bool value)
{
  suppress_events = true;
  check->setChecked(value);
  suppress_events = false;
}

void main_window::showEvent(QShowEvent * event)
{
  Q_UNUSED(event);
  if (!start_event_reported)
  {
    start_event_reported = true;
    controller->start();
  }
}

void main_window::closeEvent(QCloseEvent * event)
{
  if (!controller->exit())
  {
    // User canceled exit when prompted about settings that have not been applied.
    event->ignore();
  }
}

void main_window::on_disconnect_action_triggered()
{
  controller->disconnect_device();
}

void main_window::on_reload_settings_action_triggered()
{
  controller->reload_settings();
}

void main_window::on_restore_defaults_action_triggered()
{
  controller->restore_default_settings();
}

void main_window::on_update_timer_timeout()
{
  controller->update();
}

void main_window::on_device_name_value_linkActivated()
{
  on_documentation_action_triggered();
}

void main_window::on_documentation_action_triggered()
{
  QDesktopServices::openUrl(QUrl(DOCUMENTATION_URL));
}

void main_window::on_about_action_triggered()
{
  QMessageBox::about(this, tr("About") + " " + windowTitle(),
    QString("<h2>") + windowTitle() + "</h2>" +
    tr("<h4>Version %1</h4>"
      "<h4>Copyright &copy; %2 Pololu Corporation</h4>"
      "<p>See LICENSE.html for copyright and license information.</p>"
      "<p><a href=\"%3\">Online documentation</a></p>")
    .arg(SOFTWARE_VERSION_STRING, SOFTWARE_YEAR, DOCUMENTATION_URL));
}

void main_window::on_device_list_value_currentIndexChanged(int index)
{
  if (suppress_events) { return; }

  if (controller->disconnect_device())
  {
    QString id = device_list_value->itemData(index).toString();
    if (!id.isEmpty())
    {
      controller->connect_device_with_os_id(id.toStdString());
    }
  }
  else
  {
    // User canceled disconnect when prompted about settings that have not been
    // applied. Reset the selected device.
    controller->handle_model_changed();
  }
}

void main_window::on_disable_driver_button_clicked()
{
  controller->disable_driver();
}

void main_window::on_enable_driver_button_clicked()
{
  controller->enable_driver();
}

void main_window::on_errors_reset_counts_button_clicked()
{
  reset_error_counts();
}

void main_window::on_manual_target_position_mode_radio_toggled(bool checked)
{
  if (suppress_events) { return; }
  if (checked)
  {
    set_target_button->setText(tr("Set target position"));
  }
  else
  {
    set_target_button->setText(tr("Set target speed"));
  }
  set_manual_target(0);
  controller->set_current_position(0);
}

void main_window::on_manual_target_scroll_bar_valueChanged(int value)
{
  if (suppress_events) { return; }
  manual_target_entry_value->setValue(value);
}

void main_window::on_manual_target_scroll_bar_sliderReleased()
{
  if (suppress_events) { return; }
  if (auto_zero_target_check->isChecked())
  {
    manual_target_scroll_bar->setValue(0);
  }
}

void main_window::on_manual_target_entry_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  manual_target_scroll_bar->setValue(value);

  if (auto_set_target_check->isChecked())
  {
    on_set_target_button_clicked();
  }
}

void main_window::on_manual_target_return_key_shortcut_activated()
{
  if (manual_target_entry_value->hasFocus())
  {
    manual_target_entry_value->interpretText();
    manual_target_entry_value->selectAll();
  }
  on_set_target_button_clicked();
}

void main_window::on_set_target_button_clicked()
{
  if (manual_target_position_mode_radio->isChecked())
  {
    controller->set_target_position(manual_target_entry_value->value());
  }
  else
  {
    controller->set_target_velocity(manual_target_entry_value->value());
  }
}

void main_window::on_auto_set_target_check_stateChanged(int state)
{
  if (suppress_events) { return; }
  if (state == Qt::Checked)
  {
    on_set_target_button_clicked();
    auto_zero_target_check->setEnabled(true);
  }
  else
  {
    auto_zero_target_check->setEnabled(false);
    auto_zero_target_check->setChecked(false);
  }
}

void main_window::on_stop_button_clicked()
{
  controller->stop_motor();
}

void main_window::on_apply_settings_action_triggered()
{
  controller->apply_settings();
}

void main_window::on_control_mode_value_currentIndexChanged(int index)
{
  if (suppress_events) { return; }
  uint8_t control_mode = control_mode_value->itemData(index).toUInt();
  controller->handle_control_mode_input(control_mode);
}

void main_window::on_serial_baud_rate_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  controller->handle_serial_baud_rate_input(value);
}

void main_window::on_serial_baud_rate_value_editingFinished()
{
  if (suppress_events) { return; }
  controller->handle_serial_baud_rate_input_finished();
}

void main_window::on_serial_device_number_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  controller->handle_serial_device_number_input(value);
}

void main_window::on_serial_crc_enabled_check_stateChanged(int state)
{
  if (suppress_events) { return; }
  controller->handle_serial_crc_enabled_input(state == Qt::Checked);
}

void main_window::on_input_min_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  controller->handle_input_min_input(value);
}

void main_window::on_input_neutral_min_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  controller->handle_input_neutral_min_input(value);
}

void main_window::on_input_neutral_max_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  controller->handle_input_neutral_max_input(value);
}

void main_window::on_input_max_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  controller->handle_input_max_input(value);
}

void main_window::on_output_min_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  controller->handle_output_min_input(value);
}

void main_window::on_output_max_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  controller->handle_output_max_input(value);
}

void main_window::on_input_averaging_enabled_check_stateChanged(int state)
{
  if (suppress_events) { return; }
  controller->handle_input_averaging_enabled_input(state == Qt::Checked);
}

void main_window::on_input_hysteresis_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  controller->handle_input_hysteresis_input(value);
}

void main_window::on_encoder_prescaler_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  controller->handle_encoder_prescaler_input(value);
}

void main_window::on_encoder_postscaler_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  controller->handle_encoder_postscaler_input(value);
}

void main_window::on_encoder_unlimited_check_stateChanged(int state)
{
  if (suppress_events) { return; }
  controller->handle_encoder_unlimited_input(state == Qt::Checked);
}

void main_window::on_speed_max_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  controller->handle_speed_max_input(value);
}

void main_window::on_speed_min_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  controller->handle_speed_min_input(value);
}

void main_window::on_accel_max_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  controller->handle_accel_max_input(value);
}

void main_window::on_decel_max_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  controller->handle_decel_max_input(value);
}

void main_window::on_step_mode_value_currentIndexChanged(int index)
{
  if (suppress_events) { return; }
  uint8_t step_mode = step_mode_value->itemData(index).toUInt();
  controller->handle_step_mode_input(step_mode);
}

void main_window::on_current_limit_value_valueChanged(int value)
{
  if (suppress_events) { return; }
  controller->handle_current_limit_input(value);
}

void main_window::on_current_limit_value_editingFinished()
{
  if (suppress_events) { return; }
  controller->handle_current_limit_input_finished();
}

void main_window::on_decay_mode_value_currentIndexChanged(int index)
{
  if (suppress_events) { return; }
  uint8_t decay_mode = decay_mode_value->itemData(index).toUInt();
  controller->handle_decay_mode_input(decay_mode);
}

void main_window::on_disable_safe_start_check_stateChanged(int state)
{
  if (suppress_events) { return; }
  controller->handle_disable_safe_start_input(state == Qt::Checked);
}

void main_window::on_ignore_err_line_high_check_stateChanged(int state)
{
  if (suppress_events) { return; }
  controller->handle_ignore_err_line_high_input(state == Qt::Checked);
}

// On Mac OS X, field labels are usually right-aligned.
#ifdef __APPLE__
#define FIELD_LABEL_ALIGNMENT Qt::AlignRight
#define INDENT(s) (((s) + std::string("    ")).c_str())
#else
#define FIELD_LABEL_ALIGNMENT Qt::AlignLeft
#define INDENT(s) ((std::string("    ") + (s)).c_str())
#endif

static void setup_read_only_text_field(QGridLayout * layout, int row,
  QLabel ** label, QLabel ** value)
{
  QLabel * new_value = new QLabel();
  new_value->setTextInteractionFlags(Qt::TextSelectableByMouse);

  QLabel * new_label = new QLabel();
  new_label->setBuddy(new_value);

  layout->addWidget(new_label, row, 0, FIELD_LABEL_ALIGNMENT);
  layout->addWidget(new_value, row, 1);

  if (label) { *label = new_label; }
  if (value) { *value = new_value; }
}

static void setup_error_row(QGridLayout * layout, int row, error_row & line)
{
  // TODO make sure this all looks good on mac/linux/high dpi
  line.count = 0;
  line.name_label = new QLabel();
  // Add left margin to offset from edge of row background fill.
  line.name_label->setContentsMargins(
    line.name_label->style()->pixelMetric(QStyle::PM_LayoutLeftMargin), 0, 0, 0);
  line.stopping_value = new QLabel();
  line.stopping_value->setAlignment(Qt::AlignCenter);
  line.count_value = new QLabel();
  // Add right margin to offset from edge of row background fill.
  line.count_value->setContentsMargins(
    0, 0, line.count_value->style()->pixelMetric(QStyle::PM_LayoutRightMargin), 0);
  line.background = new QFrame();
  if (row & 1)
  {
    line.background->setStyleSheet("QFrame { background-color: palette(alternate-base); }");
  }

  // Increase the width of the Yes/No label to make it have a good width when
  // highlighted red. Increase the minimum height of the row in the layout to
  // make up for the vertical spacing being removed.
  {
    QLabel tmp_label;
    tmp_label.setText("Yes");
    line.stopping_value->setMinimumWidth(tmp_label.sizeHint().width() +
      2 * line.stopping_value->fontMetrics().height());
    layout->setRowMinimumHeight(row, tmp_label.sizeHint().height() +
      line.background->style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing));
  }

  layout->addWidget(line.background, row, 0, 1, 3);
  layout->addWidget(line.name_label, row, 0, FIELD_LABEL_ALIGNMENT);
  layout->addWidget(line.stopping_value, row, 1, Qt::AlignCenter);
  layout->addWidget(line.count_value, row, 2, Qt::AlignLeft);
}

void main_window::setup_window()
{
  // Make buttons a little bit bigger so they're easier to click.
  // TODO: do we only want this on certain buttons?
  setStyleSheet("QPushButton { padding: 0.3em 1em; }");

  setup_menu_bar();

  central_widget = new QWidget();
  QVBoxLayout * layout = central_widget_layout = new QVBoxLayout();

  layout->addLayout(setup_header());
  layout->addWidget(setup_tab_widget());
  layout->addLayout(setup_footer());

  central_widget->setLayout(layout);
  setCentralWidget(central_widget);

  retranslate();

  // Make the window non-resizable.
  //setFixedSize(sizeHint());

  program_icon = QIcon(":app_icon");
  setWindowIcon(program_icon);

  update_timer = new QTimer(this);
  update_timer->setObjectName("update_timer");

  QMetaObject::connectSlotsByName(this);
}

void main_window::setup_menu_bar()
{
  menu_bar = new QMenuBar();

  file_menu = menu_bar->addMenu("");

  exit_action = new QAction(this);
  exit_action->setObjectName("exit_action");
  exit_action->setShortcut(QKeySequence::Quit);
  connect(exit_action, SIGNAL(triggered()), this, SLOT(close()));
  file_menu->addAction(exit_action);

  device_menu = menu_bar->addMenu("");

  disconnect_action = new QAction(this);
  disconnect_action->setObjectName("disconnect_action");
  disconnect_action->setShortcut(Qt::CTRL + Qt::Key_D);
  device_menu->addAction(disconnect_action);

  device_menu->addSeparator();

  reload_settings_action = new QAction(this);
  reload_settings_action->setObjectName("reload_settings_action");
  device_menu->addAction(reload_settings_action);

  restore_defaults_action = new QAction(this);
  restore_defaults_action->setObjectName("restore_defaults_action");
  device_menu->addAction(restore_defaults_action);

  apply_settings_action = new QAction(this);
  apply_settings_action->setObjectName("apply_settings_action");
  apply_settings_action->setShortcut(Qt::CTRL + Qt::Key_P);
  device_menu->addAction(apply_settings_action);

  help_menu = menu_bar->addMenu("");

  documentation_action = new QAction(this);
  documentation_action->setObjectName("documentation_action");
  documentation_action->setShortcut(QKeySequence::HelpContents);
  help_menu->addAction(documentation_action);

  about_action = new QAction(this);
  about_action->setObjectName("about_action");
  about_action->setShortcut(QKeySequence::WhatsThis);
  help_menu->addAction(about_action);

  setMenuBar(menu_bar);
}

QLayout * main_window::setup_header()
{
  QHBoxLayout * layout = header_layout = new QHBoxLayout();

  device_list_label = new QLabel();
  device_list_value = new QComboBox();
  device_list_value->setObjectName("device_list_value");
  device_list_value->addItem(tr("Not connected"), QString()); // null value
  connection_status_value = new QLabel();

  // Make the device list wide enough to display the short name and serial
  // number of the Tic.
  {
    QComboBox tmp_box;
    tmp_box.addItem("TXXXXX: #1234567890123456");
    device_list_value->setMinimumWidth(tmp_box.sizeHint().width() * 105 / 100);
  }

  layout->addWidget(device_list_label);
  layout->addWidget(device_list_value);
  layout->addWidget(connection_status_value, 1, Qt::AlignLeft);

  return header_layout;
}

QWidget * main_window::setup_tab_widget()
{
  tab_widget = new QTabWidget();

  tab_widget->addTab(setup_status_page_widget(), tr("Status"));
  tab_widget->addTab(setup_settings_page_widget(), tr("Settings"));

  return tab_widget;
}

//// status page

QWidget * main_window::setup_status_page_widget()
{
  status_page_widget = new QWidget();
  QHBoxLayout * layout = status_page_layout = new QHBoxLayout();

  layout->addLayout(setup_status_left_column());
  layout->addLayout(setup_status_right_column());
  layout->addStretch(1);

  status_page_widget->setLayout(layout);
  return status_page_widget;
}

QLayout * main_window::setup_status_left_column()
{
  QVBoxLayout * layout = status_left_column_layout = new QVBoxLayout();

  layout->addWidget(setup_device_info_box());
  layout->addWidget(setup_manual_target_box());
  layout->addStretch(1);

  return status_left_column_layout;
}

QLayout * main_window::setup_status_right_column()
{
  QVBoxLayout * layout = status_right_column_layout = new QVBoxLayout();

  layout->addWidget(setup_status_box());
  layout->addWidget(setup_errors_box());
  layout->addStretch(1);

  return status_right_column_layout;
}

QWidget * main_window::setup_device_info_box()
{
  device_info_box = new QGroupBox();
  QGridLayout * layout = device_info_box_layout = new QGridLayout();
  layout->setColumnStretch(1, 1);
  int row = 0;

  setup_read_only_text_field(layout, row++, &device_name_label, &device_name_value);
  device_name_value->setObjectName("device_name_value");
  device_name_value->setTextInteractionFlags(Qt::TextBrowserInteraction);

  setup_read_only_text_field(layout, row++, &serial_number_label, &serial_number_value);
  setup_read_only_text_field(layout, row++, &firmware_version_label, &firmware_version_value);

  // Make the right column wide enough to display the name of the Tic,
  // which should be the widest thing that needs to fit in that column.
  // This is important for making sure that the sizeHint of the overall main
  // window has a good width before we set the window to be a fixed size.
  {
    QLabel tmpLabel;
    tmpLabel.setText("Tic USB Stepper Motor Controller TXXXXX");
    layout->setColumnMinimumWidth(1, tmpLabel.sizeHint().width());
  }

  device_info_box->setLayout(layout);
  return device_info_box;
}

QWidget * main_window::setup_status_box()
{
  status_box = new QGroupBox();
  QGridLayout * layout = status_box_layout = new QGridLayout();
  layout->setColumnStretch(1, 1);
  int row = 0;

  setup_read_only_text_field(layout, row++, &vin_voltage_label, &vin_voltage_value);
  setup_read_only_text_field(layout, row++, &target_label, &target_value);
  setup_read_only_text_field(layout, row++, &current_position_label, &current_position_value);
  setup_read_only_text_field(layout, row++, &current_velocity_label, &current_velocity_value);

  // Make the right column wide enough to display the largest possible current
  // velocity.
  {
    QLabel tmp_label;
    tmp_label.setText(QString((std::to_string(TIC_MAX_ALLOWED_SPEED) +
      " (" + convert_speed_to_pps_string(TIC_MAX_ALLOWED_SPEED) + ")").c_str()));
    layout->setColumnMinimumWidth(1, tmp_label.sizeHint().width());
  }

  status_box->setLayout(layout);
  return status_box;
}

QWidget * main_window::setup_errors_box()
{
  errors_box = new QGroupBox();
  QVBoxLayout * layout = errors_box_layout = new QVBoxLayout();

  layout->addLayout(setup_error_table_layout());

  {
    errors_reset_counts_button = new QPushButton;
    errors_reset_counts_button->setObjectName("errors_reset_counts_button");
    layout->addWidget(errors_reset_counts_button, 0, Qt::AlignRight);
  }

  reset_error_counts();

  errors_box->setLayout(layout);
  return errors_box;
}

QLayout * main_window::setup_error_table_layout()
{
  QGridLayout * layout = error_table_layout = new QGridLayout();
  layout->setHorizontalSpacing(fontMetrics().height());
  // Remove spaces between rows so row background fill looks good.
  layout->setVerticalSpacing(0);
  int row = 0;

   {
     errors_stopping_header_label = new QLabel;
     errors_count_header_label = new QLabel;
     layout->addWidget(errors_stopping_header_label, row, 1, Qt::AlignCenter);
     layout->addWidget(errors_count_header_label, row, 2, Qt::AlignLeft);
     row++;
   }

  setup_error_row(layout, row++, error_rows[TIC_ERROR_INTENTIONALLY_DISABLED]);
  setup_error_row(layout, row++, error_rows[TIC_ERROR_MOTOR_DRIVER_ERROR]);
  setup_error_row(layout, row++, error_rows[TIC_ERROR_LOW_VIN]);
  setup_error_row(layout, row++, error_rows[TIC_ERROR_KILL_SWITCH]);
  setup_error_row(layout, row++, error_rows[TIC_ERROR_REQUIRED_INPUT_INVALID]);
  setup_error_row(layout, row++, error_rows[TIC_ERROR_COMMAND_TIMEOUT]);
  setup_error_row(layout, row++, error_rows[TIC_ERROR_SAFE_START_VIOLATION]);
  setup_error_row(layout, row++, error_rows[TIC_ERROR_ERR_LINE_HIGH]);
  setup_error_row(layout, row++, error_rows[TIC_ERROR_SERIAL_ERROR]);
  setup_error_row(layout, row++, error_rows[TIC_ERROR_SERIAL_FRAMING]);
  setup_error_row(layout, row++, error_rows[TIC_ERROR_SERIAL_RX_OVERRUN]);
  setup_error_row(layout, row++, error_rows[TIC_ERROR_SERIAL_FORMAT]);
  setup_error_row(layout, row++, error_rows[TIC_ERROR_SERIAL_CRC]);
  setup_error_row(layout, row++, error_rows[TIC_ERROR_ENCODER_SKIP]);

  // Adjust height of header row to match error rows.
  layout->setRowMinimumHeight(0, layout->rowMinimumHeight(1));

  // Make the right column wide enough to display the largest possible count.
  {
    QLabel tmp_label;
    tmp_label.setText(QString::number(UINT_MAX));
    layout->setColumnMinimumWidth(2, tmp_label.sizeHint().width());
  }

  return layout;
}

QWidget * main_window::setup_manual_target_box()
{
  manual_target_box = new QGroupBox();
  QVBoxLayout * layout = manual_target_box_layout = new QVBoxLayout();

  layout->addLayout(setup_manual_target_mode_layout());

  layout->addSpacing(fontMetrics().height());

  layout->addWidget(setup_manual_target_entry_widget());

  {
    set_target_button = new QPushButton();
    set_target_button->setObjectName("set_target_button");
    layout->addWidget(set_target_button, 0, Qt::AlignCenter);
  }

  layout->addSpacing(fontMetrics().height());

  {
    auto_set_target_check = new QCheckBox();
    auto_set_target_check->setObjectName("auto_set_target_check");
    auto_set_target_check->setChecked(true);
    layout->addWidget(auto_set_target_check, 0, Qt::AlignLeft);
  }

  {
    auto_zero_target_check = new QCheckBox();
    layout->addWidget(auto_zero_target_check, 0, Qt::AlignLeft);
  }

  {
    stop_button = new QPushButton();
    stop_button->setObjectName("stop_button");
    layout->addWidget(stop_button, 0, Qt::AlignCenter);
  }

  manual_target_box->setLayout(layout);
  return manual_target_box;
}

QLayout * main_window::setup_manual_target_mode_layout()
{
  QHBoxLayout * layout = manual_target_mode_layout = new QHBoxLayout();

  manual_target_position_mode_radio = new QRadioButton();
  manual_target_position_mode_radio->setObjectName("manual_target_position_mode_radio");
  manual_target_position_mode_radio->setChecked(true);
  manual_target_speed_mode_radio = new QRadioButton();
  manual_target_speed_mode_radio->setObjectName("manual_target_speed_mode_radio");
  layout->addWidget(manual_target_position_mode_radio, 1, Qt::AlignRight);
  layout->addSpacing(fontMetrics().height());
  layout->addWidget(manual_target_speed_mode_radio, 1, Qt::AlignLeft);

  return manual_target_mode_layout;
}

QWidget * main_window::setup_manual_target_entry_widget()
{
  // This is a widget instead of a layout so that it can be a parent to the
  // shortcuts, allowing the shortcuts to work on both the scroll bar and the
  // spin box.

  manual_target_entry_widget = new QWidget();
  QGridLayout * layout = manual_target_entry_widget_layout = new QGridLayout();
  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(2, 1);
  layout->setContentsMargins(0, 0, 0, 0);
  int row = 0;

  {
    manual_target_scroll_bar = new QScrollBar(Qt::Horizontal);
    manual_target_scroll_bar->setObjectName("manual_target_scroll_bar");
    manual_target_scroll_bar->setSingleStep(1);
    manual_target_scroll_bar->setFocusPolicy(Qt::ClickFocus);
    layout->addWidget(manual_target_scroll_bar, row, 0, 1, 3);
    row++;
  }

  {
    manual_target_min_label = new QLabel();
    manual_target_max_label = new QLabel();
    manual_target_entry_value = new QSpinBox();
    manual_target_entry_value->setObjectName("manual_target_entry_value");
    // Don't emit valueChanged while user is typing (e.g. if the user enters 500,
    // we don't want to set speeds of 5, 50, and 500).
    manual_target_entry_value->setKeyboardTracking(false);


    // Make the spin box wide enough to display the largest possible target value.
    {
      QSpinBox tmp_box;
      tmp_box.setMinimum(-0x7FFFFFF);
      manual_target_entry_value->setMinimumWidth(tmp_box.sizeHint().width());
    }

    layout->addWidget(manual_target_min_label, row, 0, Qt::AlignLeft);
    layout->addWidget(manual_target_entry_value, row, 1);
    layout->addWidget(manual_target_max_label, row, 2, Qt::AlignRight);
    row++;
  }

  {
    manual_target_return_key_shortcut = new QShortcut(manual_target_entry_widget);
    manual_target_return_key_shortcut->setObjectName("manual_target_return_key_shortcut");
    manual_target_return_key_shortcut->setContext(Qt::WidgetWithChildrenShortcut);
    manual_target_return_key_shortcut->setKey(Qt::Key_Return);
    manual_target_enter_key_shortcut = new QShortcut(manual_target_entry_widget);
    manual_target_enter_key_shortcut->setObjectName("manual_target_enter_key_shortcut");
    manual_target_enter_key_shortcut->setContext(Qt::WidgetWithChildrenShortcut);
    manual_target_enter_key_shortcut->setKey(Qt::Key_Enter);

    connect(manual_target_enter_key_shortcut, SIGNAL(activated()), this,
      SLOT(on_manual_target_return_key_shortcut_activated()));
  }

  manual_target_entry_widget->setLayout(layout);
  return manual_target_entry_widget;
}

//// settings page

QWidget * main_window::setup_settings_page_widget()
{
  settings_page_widget = new QWidget();
  QHBoxLayout * layout = settings_page_layout = new QHBoxLayout();

  layout->addLayout(setup_settings_left_column());
  layout->addLayout(setup_settings_right_column());
  layout->addStretch(1);

  settings_page_widget->setLayout(layout);
  return settings_page_widget;
}

QLayout * main_window::setup_settings_left_column()
{
  QVBoxLayout * layout = settings_left_column_layout = new QVBoxLayout();

  layout->addWidget(setup_control_mode_widget());
  layout->addWidget(setup_serial_settings_box());
  layout->addWidget(setup_scaling_settings_box());
  layout->addStretch(1);

  return settings_left_column_layout;
}

QLayout * main_window::setup_settings_right_column()
{
  QVBoxLayout * layout = settings_right_column_layout = new QVBoxLayout();

  layout->addWidget(setup_motor_settings_box());
  layout->addWidget(setup_misc_settings_box());
  layout->addStretch(1);

  return settings_right_column_layout;
}

// [all-settings]

QWidget * main_window::setup_control_mode_widget()
{
  control_mode_widget = new QWidget();
  QGridLayout * layout = control_mode_widget_layout = new QGridLayout();
  layout->setColumnStretch(1, 1);
  layout->setContentsMargins(0, 0, 0, 0);
  int row = 0;

  {
    control_mode_value = new QComboBox();
    control_mode_value->setObjectName("control_mode_value");
    control_mode_value->addItem(u8"Serial\u2009/\u2009I\u00B2C\u2009/\u2009USB", TIC_CONTROL_MODE_SERIAL);
    control_mode_value->addItem("RC position", TIC_CONTROL_MODE_RC_POSITION);
    control_mode_value->addItem("RC speed", TIC_CONTROL_MODE_RC_SPEED);
    control_mode_value->addItem("Analog position", TIC_CONTROL_MODE_ANALOG_POSITION);
    control_mode_value->addItem("Analog speed", TIC_CONTROL_MODE_ANALOG_SPEED);
    control_mode_value->addItem("Encoder position", TIC_CONTROL_MODE_ENCODER_POSITION);
    control_mode_value->addItem("Encoder speed", TIC_CONTROL_MODE_ENCODER_SPEED);
    control_mode_value->addItem("STEP/DIR", TIC_CONTROL_MODE_STEP_DIR);
    control_mode_label = new QLabel();
    control_mode_label->setBuddy(control_mode_value);
    layout->addWidget(control_mode_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(control_mode_value, row, 1, Qt::AlignLeft);
    row++;
  }

  control_mode_widget->setLayout(layout);
  return control_mode_widget;
}

QWidget * main_window::setup_serial_settings_box()
{
  serial_settings_box = new QGroupBox();
  QGridLayout * layout = serial_settings_box_layout = new QGridLayout();
  layout->setColumnStretch(1, 1);
  int row = 0;

  {
    serial_baud_rate_value = new QSpinBox();
    serial_baud_rate_value->setObjectName("serial_baud_rate_value");
    serial_baud_rate_value->setRange(TIC_MIN_ALLOWED_BAUD_RATE, TIC_MAX_ALLOWED_BAUD_RATE);
    serial_baud_rate_label = new QLabel();
    serial_baud_rate_label->setBuddy(serial_baud_rate_value);
    layout->addWidget(serial_baud_rate_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(serial_baud_rate_value, row, 1, Qt::AlignLeft);
    row++;
  }

  {
    serial_device_number_value = new QSpinBox();
    serial_device_number_value->setObjectName("serial_device_number_value");
    serial_device_number_value->setRange(0, 0x7F);
    serial_device_number_label = new QLabel();
    serial_device_number_label->setBuddy(serial_device_number_value);
    layout->addWidget(serial_device_number_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(serial_device_number_value, row, 1, Qt::AlignLeft);
    row++;
  }

  {
    serial_crc_enabled_check = new QCheckBox();
    serial_crc_enabled_check->setObjectName("serial_crc_enabled_check");
    layout->addWidget(serial_crc_enabled_check, row, 0, 1, 2, Qt::AlignLeft);
    row++;
  }

  serial_settings_box->setLayout(layout);
  return serial_settings_box;
}

QWidget * main_window::setup_scaling_settings_box()
{
  scaling_settings_box = new QGroupBox();
  QGridLayout * layout = scaling_settings_box_layout = new QGridLayout();
  layout->setColumnStretch(2, 1);
  int row = 0;

  {
    scaling_input_label = new QLabel();
    scaling_target_label = new QLabel();
    layout->addWidget(scaling_input_label, row, 1, Qt::AlignLeft);
    layout->addWidget(scaling_target_label, row, 2, Qt::AlignLeft);
    row++;
  }

  {
    scaling_max_label = new QLabel();
    input_max_value = new QSpinBox();
    input_max_value->setObjectName("input_max_value");
    input_max_value->setRange(0, 0xFFFF);
    output_max_value = new QSpinBox();
    output_max_value->setObjectName("output_max_value");
    output_max_value->setRange(0, 0x7FFFFFFF);
    layout->addWidget(scaling_max_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(input_max_value, row, 1, Qt::AlignLeft);
    layout->addWidget(output_max_value, row, 2, Qt::AlignLeft);
    row++;
  }

  {
    scaling_neutral_max_label = new QLabel();
    input_neutral_max_value = new QSpinBox();
    input_neutral_max_value->setObjectName("input_neutral_max_value");
    input_neutral_max_value->setRange(0, 0xFFFF);
    layout->addWidget(scaling_neutral_max_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(input_neutral_max_value, row, 1, Qt::AlignLeft);
    row++;
  }

  {
    scaling_neutral_min_label = new QLabel();
    input_neutral_min_value = new QSpinBox();
    input_neutral_min_value->setObjectName("input_neutral_min_value");
    input_neutral_min_value->setRange(0, 0xFFFF);
    layout->addWidget(scaling_neutral_min_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(input_neutral_min_value, row, 1, Qt::AlignLeft);
    row++;
  }

  {
    scaling_min_label = new QLabel();
    input_min_value = new QSpinBox();
    input_min_value->setObjectName("input_min_value");
    input_min_value->setRange(0, 0xFFFF);
    output_min_value = new QSpinBox();
    output_min_value->setObjectName("output_min_value");
    output_min_value->setRange(-0x7FFFFFFF, 0);
    layout->addWidget(scaling_min_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(input_min_value, row, 1, Qt::AlignLeft);
    layout->addWidget(output_min_value, row, 2, Qt::AlignLeft);
    row++;
  }

  {
    input_averaging_enabled_check = new QCheckBox();
    input_averaging_enabled_check->setObjectName("input_averaging_enabled_check");
    layout->addWidget(input_averaging_enabled_check, row, 0, 1, 3, Qt::AlignLeft);
    row++;
  }

  {
    input_hysteresis_value = new QSpinBox();
    input_hysteresis_value->setObjectName("input_hysteresis_value");
    input_hysteresis_value->setRange(0, 0xFFFF);
    input_hysteresis_label = new QLabel();
    input_hysteresis_label->setBuddy(input_hysteresis_value);
    layout->addWidget(input_hysteresis_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(input_hysteresis_value, row, 1, 1, 2, Qt::AlignLeft);
    row++;
  }

  {
    encoder_prescaler_value = new QSpinBox();
    encoder_prescaler_value->setObjectName("encoder_prescaler_value");
    encoder_prescaler_value->setRange(0, 0x7FFFFFFF);
    encoder_prescaler_label = new QLabel();
    encoder_prescaler_label->setBuddy(encoder_prescaler_value);
    layout->addWidget(encoder_prescaler_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(encoder_prescaler_value, row, 1, 1, 2, Qt::AlignLeft);
    row++;
  }

  {
    encoder_postscaler_value = new QSpinBox();
    encoder_postscaler_value->setObjectName("encoder_postscaler_value");
    encoder_postscaler_value->setRange(0, 0x7FFFFFFF);
    encoder_postscaler_label = new QLabel();
    encoder_postscaler_label->setBuddy(encoder_postscaler_value);
    layout->addWidget(encoder_postscaler_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(encoder_postscaler_value, row, 1, 1, 2, Qt::AlignLeft);
    row++;
  }

  {
    encoder_unlimited_check = new QCheckBox();
    encoder_unlimited_check->setObjectName("encoder_unlimited_check");
    layout->addWidget(encoder_unlimited_check, row, 0, 1, 3, Qt::AlignLeft);
    row++;
  }

  scaling_settings_box->setLayout(layout);
  return scaling_settings_box;
}

QWidget * main_window::setup_motor_settings_box()
{
  motor_settings_box = new QGroupBox();
  QGridLayout * layout = motor_settings_box_layout = new QGridLayout();
  layout->setColumnStretch(2, 2);
  int row = 0;

  {
    speed_max_value = new QSpinBox();
    speed_max_value->setObjectName("speed_max_value");
    speed_max_value->setRange(0, TIC_MAX_ALLOWED_SPEED);
    speed_max_label = new QLabel();
    speed_max_label->setBuddy(speed_max_value);
    speed_max_value_pretty = new QLabel();
    layout->addWidget(speed_max_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(speed_max_value, row, 1, Qt::AlignLeft);
    layout->addWidget(speed_max_value_pretty, row, 2, Qt::AlignLeft);
    row++;
  }

  {
    speed_min_value = new QSpinBox();
    speed_min_value->setObjectName("speed_min_value");
    speed_min_value->setRange(0, TIC_MAX_ALLOWED_SPEED);
    speed_min_label = new QLabel();
    speed_min_label->setBuddy(speed_min_value);
    speed_min_value_pretty = new QLabel();
    layout->addWidget(speed_min_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(speed_min_value, row, 1, Qt::AlignLeft);
    layout->addWidget(speed_min_value_pretty, row, 2, Qt::AlignLeft);
    row++;
  }

  {
    accel_max_value = new QSpinBox();
    accel_max_value->setObjectName("accel_max_value");
    accel_max_value->setRange(TIC_MIN_ALLOWED_ACCEL, TIC_MAX_ALLOWED_ACCEL);
    accel_max_label = new QLabel();
    accel_max_label->setBuddy(accel_max_value);
    accel_max_value_pretty = new QLabel();
    layout->addWidget(accel_max_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(accel_max_value, row, 1, Qt::AlignLeft);
    layout->addWidget(accel_max_value_pretty, row, 2, Qt::AlignLeft);

    // Make the right column wide enough to display the largest possible pretty
    // values.
    {
      QLabel tmp_label;
      tmp_label.setText(QString(convert_accel_to_pps2_string(0x7FFFFFF).c_str()));
      layout->setColumnMinimumWidth(2, tmp_label.sizeHint().width());
    }

    row++;
  }

  {
    decel_max_value = new QSpinBox();
    decel_max_value->setObjectName("decel_max_value");
    decel_max_value->setRange(0, TIC_MAX_ALLOWED_ACCEL);
    decel_max_label = new QLabel();
    decel_max_label->setBuddy(decel_max_value);
    decel_max_value_pretty = new QLabel();
    layout->addWidget(decel_max_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(decel_max_value, row, 1, Qt::AlignLeft);
    layout->addWidget(decel_max_value_pretty, row, 2, Qt::AlignLeft);
    row++;
  }

  {
    step_mode_value = new QComboBox();
    step_mode_value->setObjectName("step_mode_value");
    step_mode_value->addItem("Full step", TIC_STEP_MODE_MICROSTEP1);
    step_mode_value->addItem("1/2 step", TIC_STEP_MODE_MICROSTEP2);
    step_mode_value->addItem("1/4 step", TIC_STEP_MODE_MICROSTEP4);
    step_mode_value->addItem("1/8 step", TIC_STEP_MODE_MICROSTEP8);
    step_mode_value->addItem("1/16 step", TIC_STEP_MODE_MICROSTEP16);
    step_mode_value->addItem("1/32 step", TIC_STEP_MODE_MICROSTEP32);
    step_mode_label = new QLabel();
    step_mode_label->setBuddy(step_mode_value);
    layout->addWidget(step_mode_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(step_mode_value, row, 1, Qt::AlignLeft);
    row++;
  }

  {
    current_limit_value = new QSpinBox();
    current_limit_value->setObjectName("current_limit_value");
    current_limit_value->setRange(0, 4000);
    current_limit_value->setSuffix(" mA");
    current_limit_label = new QLabel();
    current_limit_label->setBuddy(current_limit_value);
    layout->addWidget(current_limit_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(current_limit_value, row, 1, Qt::AlignLeft);
    row++;
  }

  {
    decay_mode_value = new QComboBox();
    decay_mode_value->setObjectName("decay_mode_value");
    decay_mode_value->addItem("Mixed", TIC_DECAY_MODE_MIXED);
    decay_mode_value->addItem("Slow", TIC_DECAY_MODE_SLOW);
    decay_mode_value->addItem("Fast", TIC_DECAY_MODE_FAST);
    decay_mode_label = new QLabel();
    decay_mode_label->setBuddy(decay_mode_value);
    layout->addWidget(decay_mode_label, row, 0, FIELD_LABEL_ALIGNMENT);
    layout->addWidget(decay_mode_value, row, 1, Qt::AlignLeft);
    row++;
  }

  motor_settings_box->setLayout(layout);
  return motor_settings_box;
}

QWidget * main_window::setup_misc_settings_box()
{
  misc_settings_box = new QGroupBox();
  QGridLayout * layout = misc_settings_box_layout = new QGridLayout();
  layout->setColumnStretch(1, 1);
  int row = 0;

  {
    disable_safe_start_check = new QCheckBox();
    disable_safe_start_check->setObjectName("disable_safe_start_check");
    layout->addWidget(disable_safe_start_check, row, 0, Qt::AlignLeft);
    row++;
  }

  {
    ignore_err_line_high_check = new QCheckBox();
    ignore_err_line_high_check->setObjectName("ignore_err_line_high_check");
    layout->addWidget(ignore_err_line_high_check, row, 0, Qt::AlignLeft);
    row++;
  }

  misc_settings_box->setLayout(layout);
  return misc_settings_box;
}

QLayout * main_window::setup_footer()
{
  QHBoxLayout * layout = footer_layout = new QHBoxLayout();

  disable_driver_button = new QPushButton();
  disable_driver_button->setObjectName("disable_driver_button");
  enable_driver_button = new QPushButton();
  enable_driver_button->setObjectName("enable_driver_button");
  apply_settings_button = new QPushButton();
  connect(apply_settings_button, SIGNAL(clicked()),
    this, SLOT(on_apply_settings_action_triggered()));
  layout->addWidget(disable_driver_button);
  layout->addWidget(enable_driver_button);
  layout->addStretch(1);
  layout->addWidget(apply_settings_button);

  return footer_layout;
}

void main_window::retranslate()
{
  setWindowTitle(tr("Pololu Tic Configuration Utility"));

  file_menu->setTitle(tr("&File"));
  exit_action->setText(tr("E&xit"));
  device_menu->setTitle(tr("&Device"));
  disconnect_action->setText(tr("&Disconnect"));
  reload_settings_action->setText(tr("Re&load settings from device"));
  restore_defaults_action->setText(tr("&Restore default settings"));
  apply_settings_action->setText(tr("&Apply settings"));
  help_menu->setTitle(tr("&Help"));
  documentation_action->setText(tr("&Online documentation..."));
  about_action->setText(tr("&About..."));

  device_list_label->setText(tr("Connected to:"));

  device_info_box->setTitle(tr("Device info"));
  device_name_label->setText(tr("Name:"));
  serial_number_label->setText(tr("Serial number:"));
  firmware_version_label->setText(tr("Firmware version:"));

  status_box->setTitle(tr("Status"));
  vin_voltage_label->setText(tr("VIN voltage:"));
  set_target_none(); // TODO: display correct target mode if we retranslate on the fly
  current_position_label->setText(tr("Current position:"));
  current_velocity_label->setText(tr("Current velocity:"));

  errors_box->setTitle(tr("Errors"));
  errors_stopping_header_label->setText(tr("Stopping motor?"));
  errors_count_header_label->setText(tr("Count"));
  error_rows[TIC_ERROR_INTENTIONALLY_DISABLED].name_label->setText(tr("Intentionally disabled"));
  error_rows[TIC_ERROR_MOTOR_DRIVER_ERROR]    .name_label->setText(tr("Motor driver error"));
  error_rows[TIC_ERROR_LOW_VIN]               .name_label->setText(tr("Low VIN"));
  error_rows[TIC_ERROR_KILL_SWITCH]           .name_label->setText(tr("Kill switch"));
  error_rows[TIC_ERROR_REQUIRED_INPUT_INVALID].name_label->setText(tr("Required input invalid"));
  error_rows[TIC_ERROR_COMMAND_TIMEOUT]       .name_label->setText(tr("Command timeout"));
  error_rows[TIC_ERROR_SAFE_START_VIOLATION]  .name_label->setText(tr("Safe start violation"));
  error_rows[TIC_ERROR_ERR_LINE_HIGH]         .name_label->setText(tr("ERR line high"));
  error_rows[TIC_ERROR_SERIAL_ERROR]          .name_label->setText(tr("Serial errors:"));
  error_rows[TIC_ERROR_SERIAL_FRAMING]        .name_label->setText(tr(INDENT("Frame")));
  error_rows[TIC_ERROR_SERIAL_RX_OVERRUN]     .name_label->setText(tr(INDENT("RX overrun")));
  error_rows[TIC_ERROR_SERIAL_FORMAT]         .name_label->setText(tr(INDENT("Format")));
  error_rows[TIC_ERROR_SERIAL_CRC]            .name_label->setText(tr(INDENT("CRC")));
  error_rows[TIC_ERROR_ENCODER_SKIP]          .name_label->setText(tr("Encoder skip"));
  errors_reset_counts_button->setText(tr("Reset counts"));

  manual_target_box->setTitle(tr(u8"Set target (Serial\u2009/\u2009I\u00B2C\u2009/\u2009USB mode only)"));
  manual_target_position_mode_radio->setText(tr("Set position"));
  manual_target_speed_mode_radio->setText(tr("Set speed"));
  if (manual_target_position_mode_radio->isChecked())
  {
    set_target_button->setText(tr("Set target position"));
  }
  else
  {
    set_target_button->setText(tr("Set target speed"));
  }
  auto_set_target_check->setText(tr("Set target when slider or entry box are changed"));
  auto_zero_target_check->setText(tr("Return slider to zero when it is released"));

  // [all-settings]
  control_mode_label->setText(tr("Control mode:"));

  serial_settings_box->setTitle(tr("Serial settings"));
  serial_baud_rate_label->setText(tr("Baud rate:"));
  serial_device_number_label->setText(tr("Device number:"));
  serial_crc_enabled_check->setText(tr("Enable CRC"));

  scaling_settings_box->setTitle(tr("Input and scaling settings"));
  scaling_input_label->setText(tr("Input"));
  scaling_target_label->setText(tr("Target"));
  scaling_min_label->setText(tr("Minimum:"));
  scaling_neutral_min_label->setText(tr("Neutral min:"));
  scaling_neutral_max_label->setText(tr("Neutral max:"));
  scaling_max_label->setText(tr("Maximum:"));

  input_averaging_enabled_check->setText(tr("Enable input averaging"));
  input_hysteresis_label->setText(tr("Input hysteresis:"));
  encoder_prescaler_label->setText(tr("Encoder prescaler:"));
  encoder_postscaler_label->setText(tr("Encoder postscaler:"));
  encoder_unlimited_check->setText(tr("Enable unlimited encoder position control"));

  motor_settings_box->setTitle(tr("Motor settings"));
  speed_max_label->setText(tr("Speed max:"));
  speed_min_label->setText(tr("Speed min:"));
  accel_max_label->setText(tr("Acceleration max:"));
  decel_max_label->setText(tr("Deceleration max:"));
  step_mode_label->setText(tr("Step mode:"));
  current_limit_label->setText(tr("Current limit:"));
  decay_mode_label->setText(tr("Decay mode:"));

  misc_settings_box->setTitle(tr("Miscellaneous settings"));
  disable_safe_start_check->setText(tr("Disable safe start"));
  ignore_err_line_high_check->setText(tr("Ignore ERR line high"));

  stop_button->setText(tr("Stop motor"));
  disable_driver_button->setText(tr("Disable driver"));
  enable_driver_button->setText(tr("Enable driver"));
  apply_settings_button->setText(apply_settings_action->text());
}