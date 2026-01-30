//#define HYSTERESIS

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.IO.Ports;
using System.IO;
using System.Timers;
using Microsoft.Win32;
using System.Threading;
using System.ComponentModel;

using Abt.Controls.SciChart;
using Abt.Controls.SciChart.Model.DataSeries;
using Abt.Controls.SciChart.Utility;
using Abt.Controls.SciChart.Visuals;
using Abt.Controls.SciChart.Visuals.Annotations;
using Abt.Controls.SciChart.Visuals.Axes;
using Abt.Controls.SciChart.ChartModifiers;
using Abt.Controls.SciChart.Visuals.RenderableSeries;
using System.Windows.Threading;
using System.Diagnostics;

//using Microsoft.Office.Interop.Excel;
using Excel = Microsoft.Office.Interop.Excel;
using Button = System.Windows.Controls.Button;

using Microsoft.Office.Core;

namespace Thorlabs
{
    public partial class Piezo
    {
        public enum Modes
        {
            PZ_STOP_MODE = 0,
            PZ_ANALOG_INPUT_MODE = 1,
            SET_DAC_VOLTS = 2,
            PZ_CONTROLER_MODE = 3,
            PZ_PID_MODE = 4,
            PZ_HYSTERESIS_MODE = 5,
        }
    }

}


namespace MCM6000_UI
{
    public partial class MainWindow
    {
        public int piezo_last_slot_selected = -1;
        byte slot_encoder_on;
        //byte piezo_analog_input_mode;

        private void bt_piezo_params_refresh_Click(object sender, RoutedEventArgs e)
        {
            piezo_last_slot_selected = (byte)(Convert.ToByte(cb_piezo_select.SelectedIndex));

            if (piezo_last_slot_selected == -1)
                return;
            else
            {
                // get the stage selected from the combo  box 
                byte slot = (byte)(Convert.ToByte(cb_piezo_select.SelectedIndex) + 0x21);

                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_PIEZO_REQ_PRAMS));
                byte[] bytesToSend = new byte[6];

                bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
                usb_write("piezo_functions", bytesToSend, 6);
                Thread.Sleep(20);

                // refresh the vaules
                //  bt_piezo_values_refresh_Click(null, null);
                //Thread.Sleep(20);
                //piezo_analog_input_refresh(null, null);
            }
        }

        // return from MGMSG_PIEZO_REQ_PRAMS
        private void piezo_get_params()
        {
            byte slot = ext_data[0];
            slot_encoder_on = ext_data[2];

            Double gain_factor = ext_data[3];
            // is a byte, need to divied by 100, 1.0 default, 1.1 makes faster, .9 makes slower 
            gain_factor /= 100;


            this.Dispatcher.Invoke(new Action(() =>
            {
                cb_piezo_encoder_select.SelectedIndex = slot_encoder_on;
                tb_piezo_gain_factor.Text = gain_factor.ToString("0.00");
            }));
        }

        // MGMSG_MCM_PIEZO_SET_PRAMS
        private void save_piezo_params_Click(object sender, RoutedEventArgs e)
        {
            // get the stage selected from the combo  box 
            byte slot = (byte)((Convert.ToByte(cb_piezo_select.SelectedIndex) + 0x21) | 0x80);

            byte chan_id = (byte)(Convert.ToByte(cb_piezo_select.SelectedIndex));

            slot_encoder_on = (byte)(Convert.ToByte(cb_piezo_encoder_select.SelectedIndex));

            Double gain_factor_t = Convert.ToDouble(tb_piezo_gain_factor.Text);
            gain_factor_t *= 100; // scale to preserve the decimal point

            // set limits
            if (gain_factor_t > 160)
                gain_factor_t = 160;
            if (gain_factor_t < 20)
                gain_factor_t = 20;

            byte gain_factor = (byte)gain_factor_t;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_PIEZO_SET_PRAMS));
            byte[] bytesToSend = new byte[6 + 4] { command[0], command[1], 4, 0x00, slot, HOST_ID,
                chan_id, 0x00,                      // 0
                slot_encoder_on,                    // 2
                gain_factor
               };

            usb_write("piezo_functions", bytesToSend, 6 + 4);
            Thread.Sleep(20);
        }
        private void bt_piezo_values_refresh_Click(object sender, RoutedEventArgs e)
        {
            piezo_last_slot_selected = (byte)(Convert.ToByte(cb_piezo_select.SelectedIndex));

            if (piezo_last_slot_selected == -1)
                return;
            else
            {
                // get the stage selected from the combo  box 
                byte slot = (byte)(Convert.ToByte(cb_piezo_select.SelectedIndex) + 0x21);

                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_PIEZO_REQ_VALUES));
                byte[] bytesToSend = new byte[6];

                bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
                usb_write("piezo_functions", bytesToSend, 6);
                Thread.Sleep(20);

                // also refresh the pid values
                bt_piezo_pid_values_refresh(sender, e);
            }
        }

        // return from MGMSG_MCM_PIEZO_REQ_VALUES
        private void piezo_get_values()
        {
            byte slot = ext_data[0];

            Double Kp = (ext_data[2] | ext_data[3] << 8 | ext_data[4] << 16 | ext_data[5] << 24);
            Kp /= 1000000;

            Double displacement_time = (ext_data[6] | ext_data[7] << 8);

            Double overshoot = (ext_data[8] | ext_data[9] << 8);
            overshoot /= 100; // in um, encoder 10nm/count

            Double overshoot_time = (ext_data[10] | ext_data[11] << 8);

            this.Dispatcher.Invoke(new Action(() =>
            {
                tb_piezo_kp.Text = Kp.ToString("0.0000");
                tb_piezo_displacement_time.Text = displacement_time.ToString("0,0.0");
                tb_piezo_overshoot.Text = overshoot.ToString("0.000");
                tb_piezo_overshoot_duration.Text = overshoot_time.ToString("0.00");
            }));
        }

        private void bt_piezo_pid_values_refresh(object sender, RoutedEventArgs e)
        {
            piezo_last_slot_selected = (byte)(Convert.ToByte(cb_piezo_select.SelectedIndex));

            if (piezo_last_slot_selected == -1)
                return;
            else
            {
                // get the stage selected from the combo  box 
                byte slot = (byte)(Convert.ToByte(cb_piezo_select.SelectedIndex) + 0x21);

                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_PIEZO_REQ_PID_PARMS));
                byte[] bytesToSend = new byte[6];

                bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
                usb_write("piezo_functions", bytesToSend, 6);
                Thread.Sleep(20);
            }
        }


        private void save_piezo_pid_params_Click(object sender, RoutedEventArgs e)
        {
            piezo_last_slot_selected = (byte)(Convert.ToByte(cb_piezo_select.SelectedIndex));

            if (piezo_last_slot_selected == -1)
                return;
            else
            {
                // get the stage selected from the combo  box 
                byte slot = (byte)((Convert.ToByte(cb_piezo_select.SelectedIndex) + 0x21) | 0x80);

                byte chan_id = (byte)(Convert.ToByte(cb_piezo_select.SelectedIndex));

                byte[] Kp = BitConverter.GetBytes(Convert.ToInt32(tb_piezo_p_term.Text));
                byte[] Ki = BitConverter.GetBytes(Convert.ToInt32(tb_piezo_i_term.Text));
                byte[] Kd = BitConverter.GetBytes(Convert.ToInt32(tb_piezo_d_term.Text));
                byte[] iMax = BitConverter.GetBytes(Convert.ToInt32(tb_piezo_imax.Text));

                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_PIEZO_SET_PID_PARMS));
                byte[] bytesToSend = new byte[6 + 18] { command[0], command[1], 18, 0x00, slot, HOST_ID,
                chan_id, 0x00,
                Kp[0], Kp[1], Kp[2], Kp[3],
                Ki[0], Ki[1], Ki[2], Ki[3],
                Kd[0], Kd[1], Kd[2], Kd[3],
                iMax[0], iMax[1], iMax[2], iMax[3],
               };

                usb_write("piezo_functions", bytesToSend, 6 + 18);
                Thread.Sleep(20);
            }
        }

        // return from MGMSG_MCM_PIEZO_REQ_PID_PARMS    
        private void piezo_get_pid_params()
        {
            Int32 Kp = (ext_data[2] | ext_data[3] << 8 | ext_data[4] << 16 | ext_data[5] << 24);
            Int32 Ki = (ext_data[6] | ext_data[7] << 8 | ext_data[8] << 16 | ext_data[9] << 24);
            Int32 Kd = (ext_data[10] | ext_data[11] << 8 | ext_data[12] << 16 | ext_data[13] << 24);
            Int32 imax = (ext_data[14] | ext_data[15] << 8 | ext_data[16] << 16 | ext_data[17] << 24);

            this.Dispatcher.Invoke(new Action(() =>
            {
                cb_piezo_encoder_select.SelectedIndex = slot_encoder_on;
                tb_piezo_p_term.Text = Kp.ToString();
                tb_piezo_i_term.Text = Ki.ToString();
                tb_piezo_d_term.Text = Kd.ToString();
                tb_piezo_imax.Text = imax.ToString();
            }));
        }

        private void btSetDac_Click(object sender, RoutedEventArgs e)
        {
            // get the stage selected from the combo  box 
            byte slot = (byte)((Convert.ToByte(cb_piezo_select.SelectedIndex) + 0x21) | 0x80);

            byte chan_id = (byte)(Convert.ToByte(cb_piezo_select.SelectedIndex));

            float num = float.Parse(tbDAC_Voltage.Text);
            if ((num < 0) || (num > 5))
            {
                return;
            }

            string stringValue = num.ToString().Replace(',', '.');
            double value = Convert.ToDouble(stringValue);
            value = value * 1000; // in mV          / 0.000076295109;
            //if (value > 65535)
            //    value = 65535;

            byte[] dac_val = BitConverter.GetBytes(Convert.ToUInt16(value));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_PIEZO_SET_DAC_VOLTS));
            byte[] bytesToSend = new byte[6 + 4] { command[0], command[1], 4, 0x00, slot, HOST_ID,
                chan_id, 0x00,          // 6-7
                dac_val[0], dac_val[1], // 8
                };

            usb_write("piezo_functions", bytesToSend, 6 + 4);
            Thread.Sleep(20);
        }

        private void btPiezo_move_by_minus_Click(object sender, RoutedEventArgs e)
        {
            // get the stage selected from the combo  box 
            byte slot = (byte)((Convert.ToByte(cb_piezo_select.SelectedIndex) + 0x21) | 0x80);

            byte chan_id = (byte)(Convert.ToByte(cb_piezo_select.SelectedIndex));

            float num = float.Parse(tbPiezo_move_by.Text);
            if (num > 100)
            {
                MessageBox.Show("Value must be between 0 and 100", "Error");
                return;
            }

            string stringValue = num.ToString().Replace(',', '.');
            double value = Convert.ToDouble(stringValue);
            value = (1000 * value) / slot_nm_per_count[slot_encoder_on];
            value *= -1;

            byte[] data1 = BitConverter.GetBytes(Convert.ToInt32(value));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_PIEZO_SET_MOVE_BY));

            byte mode;
            if (cb_piezo_move_by_mode.IsChecked == false)
                mode = 0; // Use the controller mode to move the piezo
            else
                mode = 1; // Use the pid controller mode to move the piezo

            byte[] bytesToSend = new byte[6 + 6] { command[0], command[1], 6, 0x00, slot, HOST_ID,
                chan_id, mode,          // 6-7
               data1[0], data1[1], data1[2], data1[3]
                };

            usb_write("piezo_functions", bytesToSend, 6 + 6);
            Thread.Sleep(20);
        }

        private void btPiezo_move_by_plus_Click(object sender, RoutedEventArgs e)
        {
            // get the stage selected from the combo  box 
            byte slot = (byte)((Convert.ToByte(cb_piezo_select.SelectedIndex) + 0x21) | 0x80);

            byte chan_id = (byte)(Convert.ToByte(cb_piezo_select.SelectedIndex));

            float num = float.Parse(tbPiezo_move_by.Text);
            if (num > 100)
            {
                MessageBox.Show("Value must be between 0 and 100", "Error");
                return;
            }

            string stringValue = num.ToString().Replace(',', '.');
            double value = Convert.ToDouble(stringValue);
            value = (1000 * value) / slot_nm_per_count[slot_encoder_on];

            byte[] data1 = BitConverter.GetBytes(Convert.ToInt32(value));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_PIEZO_SET_MOVE_BY));

            byte mode;
            if (cb_piezo_move_by_mode.IsChecked == false)
                mode = 0; // Use the controller mode to move the piezo
            else
                mode = 1; // Use the pid controller mode to move the piezo

            byte[] bytesToSend = new byte[6 + 6] { command[0], command[1], 6, 0x00, slot, HOST_ID,
                chan_id, mode,          // 6-7
                data1[0], data1[1], data1[2], data1[3]
                };

            usb_write("piezo_functions", bytesToSend, 6 + 6);
            Thread.Sleep(20);
        }

        private void btPiezo_set_mode(byte slot, byte mode)
        {

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_PIEZO_SET_MODE));
            byte[] bytesToSend = new byte[6] { command[0], command[1], mode, 0x00, slot, HOST_ID };

            usb_write("piezo_functions", bytesToSend, 6);
            Thread.Sleep(50);
        }
        private void btPiezo_stop_Click(object sender, RoutedEventArgs e)
        {
            // get the stage selected from the combo  box 
            byte slot = (byte)((Convert.ToByte(cb_piezo_select.SelectedIndex) + 0x21));
            btPiezo_set_mode(slot, (byte)Thorlabs.Piezo.Modes.PZ_STOP_MODE);
        }
        private void btPiezo_anlog_input_Click(object sender, RoutedEventArgs e)
        {
            // get the stage selected from the combo  box 
            byte slot = (byte)((Convert.ToByte(cb_piezo_select.SelectedIndex) + 0x21));
            btPiezo_set_mode(slot, (byte)Thorlabs.Piezo.Modes.PZ_ANALOG_INPUT_MODE);
        }

        private void piezo_analog_input_refresh(object sender, RoutedEventArgs e)
        {
            piezo_last_slot_selected = (byte)(Convert.ToByte(cb_piezo_select.SelectedIndex));

            if (piezo_last_slot_selected == -1)
                return;
            else
            {
                // get the stage selected from the combo  box 
                byte slot = (byte)(Convert.ToByte(cb_piezo_select.SelectedIndex) + 0x21);

                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_PIEZO_REQ_MODE));
                byte[] bytesToSend = new byte[6];

                bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
                usb_write("piezo_functions", bytesToSend, 6);
                Thread.Sleep(20);
            }
        }

        //private void piezo_get_analog_input_mode_update_btn_text()
        //{
        //    this.Dispatcher.Invoke(new Action(() =>
        //    {
        //        switch (piezo_analog_input_mode)
        //        {
        //            case (byte)Piezo_analog_input_mode.ANALOG_INPUT_MODE_DISABLED:
        //                btPiezo_anlog_input.Content = "Analog Input Disabled";
        //                break;

        //            case (byte)Piezo_analog_input_mode.ANALOG_INPUT_MODE_ENABLED:
        //                btPiezo_anlog_input.Content = "Analog Input Enabled";
        //                break;

        //            default:
        //                btPiezo_anlog_input.Content = "Analog Input Disabled";
        //                break;
        //        }
        //    }));
        //}

        // return from MGMSG_MCM_PIEZO_REQ_ANALOG_INPUT
        //private void piezo_get_analog_input_mode()
        //{
        //    byte slot = (byte)(Convert.ToByte(header[5]) - 0x21);
        //    piezo_analog_input_mode = (byte)(Convert.ToByte(header[2]));
        //}


        // ******************** chart ********************************************//

        private bool plot_enable;
        public double ms_x_axis;
        private double _piezo_windowSize = 500;//2000;
        static DispatcherTimer timer_piezo_plot;

        public XyDataSeries<double, double> adc_Series;
        public XyDataSeries<double, double> enc_Series;
        public XyDataSeries<double, double> enc_filtered_Series;
        public XyDataSeries<double, double> dac_Series;
        public XyDataSeries<double, double> error_Series;
        public XyDataSeries<double, double> setpoint_Series;

        /*
        public event EventHandler<EventArgs> Tick;
        public bool Running { get; private set; }
        public int Interval { get; private set; }

        static DispatcherTimer timer_piezo_plot;

        public double piezo_log_interval = 10; // 1ms or 1kHz
            */
        //public double piezo_data_count = 0;
        //private double _piezo_windowSize = 2000;
        //static DispatcherTimer timer_piezo_plot;

        //public struct PiezoData
        //{
        //    public XyDataSeries<double, double> adc_Series;
        //    //public XyDataSeries<double, double> enc_Series;
        //    public double ms_x_axis;
        //    public bool plot_enable;
        //};
        //PiezoData piezo_data = new PiezoData();


        private void initialize_piezo_plots()
        {
            ms_x_axis = 0;

            // ADC voltage
            adc_Series = new XyDataSeries<double, double>();
            LineSeries_piezo_adc_Series.DataSeries = adc_Series;
            LineSeries_piezo_adc_Series.SeriesColor = Colors.Yellow;
            adc_Series.SeriesName = "ADC";

            // encoder counts
            enc_Series = new XyDataSeries<double, double>();
            LineSeries_piezo_enc_Series.DataSeries = enc_Series;
            LineSeries_piezo_enc_Series.SeriesColor = Colors.Green;
            enc_Series.SeriesName = "Enc";

            // encoder filtered counts
            enc_filtered_Series = new XyDataSeries<double, double>();
            LineSeries_piezo_enc_filtered_Series.DataSeries = enc_filtered_Series;
            LineSeries_piezo_enc_filtered_Series.SeriesColor = Colors.GreenYellow;
            enc_filtered_Series.SeriesName = "Enc FLT";

            // DAC voltage
            dac_Series = new XyDataSeries<double, double>();
            LineSeries_piezo_dac_Series.DataSeries = dac_Series;
            LineSeries_piezo_dac_Series.SeriesColor = Colors.Orange;
            dac_Series.SeriesName = "DAC";

            // PID error
            error_Series = new XyDataSeries<double, double>();
            LineSeries_piezo_error_Series.DataSeries = error_Series;
            LineSeries_piezo_error_Series.SeriesColor = Colors.Red;
            error_Series.SeriesName = "ERROR";

            // PID setpoint
            setpoint_Series = new XyDataSeries<double, double>();
            LineSeries_piezo_setpoint_Series.DataSeries = setpoint_Series;
            LineSeries_piezo_setpoint_Series.SeriesColor = Colors.Blue;
            setpoint_Series.SeriesName = "Setpoint";


            plot_enable = false;

            // PreviewMouseDown
            SciF_piezo.PreviewMouseDown += (s, arg) =>
            {
                // On mouse down (but not double click), freeze our last N seconds window 
                if (!_thatWasADoubleClick) _showLatestWindow = false;

                _thatWasADoubleClick = false;



            };

            // PreviewMouseDoubleClick
            SciF_piezo.PreviewMouseDoubleClick += (s, arg) =>
            {
                _showLatestWindow = true;
                _thatWasADoubleClick = true; // (8): Prevent contention between double click and single click event

                plot_autorange_Y_axis();


                //  SciF_piezo.XAxis.AnimateVisibleRangeTo(new DoubleRange(ms_x_axis - _piezo_windowSize, ms_x_axis), TimeSpan.FromMilliseconds(200));
            };

            // Create a timer to tick new data 
            timer_piezo_plot = new DispatcherTimer(DispatcherPriority.Render);
            timer_piezo_plot.Interval = TimeSpan.FromSeconds(1);
            timer_piezo_plot.Tick += TimerOnTickPiezo;
            timer_piezo_plot.Start();
            _piezo_windowSize = sl_plot_scroll_size.Value;



        }

        static double delta_x_ms, delta_y_us, point_1, point_2, point_1y, point_2y = 0;
        static bool first_point = true;
        private void Scs_OnMouseDown(object sender, MouseButtonEventArgs e)
        {


            // Get mouse point
            var mousePoint = e.GetPosition(SciF_piezo);

            // Transform point to inner viewport 
            // https://www.scichart.com/questions/question/how-can-i-convert-xyaxis-value-to-chart-surface-coodinate
            // https://www.scichart.com/documentation/v5.x/Axis%20APIs%20-%20Convert%20Pixel%20to%20Data%20Coordinates.html
            mousePoint = SciF_piezo.RootGrid.TranslatePoint(mousePoint, SciF_piezo.ModifierSurface);

            // Convert the mousePoint.X to x DataValue using axis
            var xDataValue = SciF_piezo.XAxis.GetDataValue(mousePoint.X);
            var yDataValue = LineSeries_piezo_enc_Series.YAxis.GetDataValue(mousePoint.Y);

            if (first_point)
            {
                point_1 = (double)xDataValue;
                point_1y = (double)yDataValue;
                first_point = false;
            }
            else
            {
                point_2 = (double)xDataValue;
                point_2y = (double)yDataValue;
                first_point = true;
            }

            delta_x_ms = Math.Abs(point_1 - point_2);
            delta_y_us = Math.Abs(point_1y - point_2y);

            this.Dispatcher.Invoke(new Action(() =>
            {
                tbPiezo_delta_ms.Text = delta_x_ms.ToString("0,0.0");
                tbPiezo_delta_y.Text = delta_y_us.ToString("0,0.000");
            }));

            // Create a vertical line annotation at the mouse point 
            //    SciF_piezo.Annotations.Add(new VerticalLineAnnotation() { X1 = xDataValue });
        }

        private void plot_autorange_Y_axis()
        {
            // Volts axis
            // Decouple the instance so you don't modify original
            var currentRange_x1 = (DoubleRange)SciF_piezo.YAxes[0].VisibleRange.AsDoubleRange().Clone();
            double shrink_x1 = currentRange_x1.Max * .15;
            double min_x1 = currentRange_x1.Min - shrink_x1;
            double max_x1 = currentRange_x1.Max + shrink_x1;
            // Restore our last N seconds window on double click
            SciF_piezo.YAxes[0].AnimateVisibleRangeTo(new DoubleRange(min_x1, max_x1), TimeSpan.FromMilliseconds(200));

            // um axis
            // Decouple the instance so you don't modify original
            var currentRange_x2 = (DoubleRange)SciF_piezo.YAxes[1].VisibleRange.AsDoubleRange().Clone();
            double shrink_x2 = currentRange_x2.Max * .15;
            double min_x2 = currentRange_x2.Min - shrink_x2;
            double max_x2 = currentRange_x2.Max + shrink_x2;
            // Restore our last N seconds window on double click
            SciF_piezo.YAxes[1].AnimateVisibleRangeTo(new DoubleRange(min_x2, max_x2), TimeSpan.FromMilliseconds(200));
        }

        private void TimerOnTickPiezo(object sender, EventArgs eventArgs)
        {
            if (piezo_last_slot_selected == -1)
            {
                return;
            }

            if (_showLatestWindow)
            {
                //   SciF_piezo.YAxes[0].AutoRange = AutoRange.Once;
                //  SciF_piezo.YAxes[1].AutoRange = AutoRange.Once;

#if !HYSTERESIS
                xPiezoAxis.AnimateVisibleRangeTo(new DoubleRange(ms_x_axis - _piezo_windowSize, ms_x_axis), TimeSpan.FromMilliseconds(200));
#endif
            }
        }

        private void OnScrollbarSelectedRangeChanged(object sender, SelectedRangeChangedEventArgs e)
        {
            // valid SelectedRangeEventTypes include:
            //  SelectedRangeEventType.ExternalSource (Axis range was updated)
            //  SelectedRangeEventType.Drag (scrollbar dragged without resizing)
            //  SelectedRangeEventType.Resize (Scrollbar was resized)
            //  SelectedRangeEventType.Moved (viewport was moved by click on the non selected area)
            //Debug.WriteLine("Scrollbar Scrolled: EventType={0}, VisibleRange={1}", e.EventType,
            //                          e.SelectedRange);
        }
        public void request_plot_piezo_values()
        {
            if (piezo_last_slot_selected == -1)
            {
                return;
            }
            else
            {
                Byte slot_id = (Byte)((piezo_last_slot_selected) + 0x21);
                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_PIEZO_REQ_VALUES));

                byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot_id, HOST_ID };
                usb_write("piezo_functions", bytesToSend, 6);

            }

        }

        double last_enc_val_cnt = 0;
        double last_enc_filtered_val_cnt = 0;
        double zerro_enc_val = 0;
        double zerro_enc_filtered_val = 0;

        public void piezo_log_update()
        {
            try
            {
#if !HYSTERESIS


                // adc value 16 bit
                double adc_val = ext_data[0] | ext_data[1] << 8;
                // convert adc value to volts adc volts/count = 0.000156250
                adc_val = adc_val * 0.000156250;
                adc_Series.Append(ms_x_axis, adc_val);
                adc_Series.Update(ms_x_axis, adc_val);

                // enc value 32 bit
                double enc_val = ext_data[2] | ext_data[3] << 8 | ext_data[4] << 16 | ext_data[5] << 24;
                enc_val = enc_val * slot_nm_per_count[slot_encoder_on] / 1e3;
                last_enc_val_cnt = enc_val;
                enc_val -= zerro_enc_val;
                enc_Series.Append(ms_x_axis, enc_val);
                enc_Series.Update(ms_x_axis, enc_val);

                // enc filtered value 32 bit
                double enc_filtered_val = ext_data[6] | ext_data[7] << 8 | ext_data[8] << 16 | ext_data[9] << 24;
                enc_filtered_val = enc_filtered_val * slot_nm_per_count[slot_encoder_on] / 1e3;
                last_enc_filtered_val_cnt = enc_filtered_val;
                enc_filtered_val -= zerro_enc_filtered_val;
                enc_filtered_Series.Append(ms_x_axis, enc_filtered_val);
                enc_filtered_Series.Update(ms_x_axis, enc_filtered_val);

                // dac value 16 bit
                double dac_val = ext_data[10] | ext_data[11] << 8;
                // convert dac value to volts adc volts/count = 0.000076293945
                dac_val = dac_val * 0.000076293945;
                dac_Series.Append(ms_x_axis, dac_val);
                dac_Series.Update(ms_x_axis, dac_val);

                // enc value 32 bit
                double error_val = ext_data[12] | ext_data[13] << 8 | ext_data[14] << 16 | ext_data[15] << 24;
                // error_val *= -1;
                error_Series.Append(ms_x_axis, error_val);
                error_Series.Update(ms_x_axis, error_val);

                // setpoint value 32 bit
                double setpoint_val = ext_data[16] | ext_data[17] << 8 | ext_data[18] << 16 | ext_data[19] << 24;
                setpoint_val = setpoint_val * slot_nm_per_count[slot_encoder_on] / 1e3;
                setpoint_val -= zerro_enc_val;
                setpoint_Series.Append(ms_x_axis, setpoint_val);
                setpoint_Series.Update(ms_x_axis, setpoint_val);

                // ms_x_axis += 1;
                ms_x_axis += .25;
                //  ms_x_axis += .2;
#else   // plot for hysteresis 

#if true //V/um

                // enc value 32 bit
                double enc_val = ext_data[2] | ext_data[3] << 8 | ext_data[4] << 16 | ext_data[5] << 24;
                enc_val = enc_val * slot_nm_per_count[slot_encoder_on] / 1e3;
                last_enc_val_cnt = enc_val;
                enc_val -= zerro_enc_val;
                ms_x_axis = enc_val;


                // dac value 16 bit
                double dac_val = ext_data[10] | ext_data[11] << 8;
                // convert dac value to volts adc volts/count = 0.000076293945
                dac_val = dac_val * 0.000076293945;
                enc_Series.Append(ms_x_axis, dac_val);
                enc_Series.Update(ms_x_axis, dac_val);

#else // um/V
                // dac value 16 bit
                double dac_val = ext_data[10] | ext_data[11] << 8;
                // convert dac value to volts adc volts/count = 0.000076293945
                dac_val = dac_val * 0.000076293945;

                ms_x_axis = dac_val;

                // enc value 32 bit
                double enc_val = ext_data[2] | ext_data[3] << 8 | ext_data[4] << 16 | ext_data[5] << 24;
                enc_val = enc_val * slot_nm_per_count[slot_encoder_on] / 1e3;
                last_enc_val_cnt = enc_val;
                enc_val -= zerro_enc_val;
                enc_Series.Append(ms_x_axis, enc_val);
                enc_Series.Update(ms_x_axis, enc_val);

#endif
#endif


            }
            catch (Exception)
            {
            }
        }

        private void Clear_piezo_Plot_Click(object sender, RoutedEventArgs e)
        {
            if (cb_piezo_select.SelectedIndex == -1)
            {
                clear_focus();
                return;
            }

            else
            {
                adc_Series.Clear();
                enc_Series.Clear();
                enc_filtered_Series.Clear();
                dac_Series.Clear();
                error_Series.Clear();
                setpoint_Series.Clear();

                ms_x_axis = 0;
                //  plot_enable = false;
                data_count = 0;
                clear_focus();
            }
        }

        private void sl_piezo_plot_scroll_size_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            _piezo_windowSize = sl_plot_scroll_size_piezo.Value;
        }

        private void cb_piezo_plot_enable_Click(object sender, RoutedEventArgs e)
        {
            Byte slot_id = (Byte)((piezo_last_slot_selected) + 0x21);
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_PIEZO_SET_ENABLE_PLOT));

            Byte enable_log_state = Convert.ToByte(cb_piezo_plot_enable.IsChecked);

            byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, enable_log_state, slot_id, HOST_ID };
            usb_write("piezo_functions", bytesToSend, 6);
        }

        private void cb_piezo_plot_disable()
        {
            for (int slot = 0; slot < NUMBER_OF_BOARD_SLOTS; slot++)
            {
                if (card_type[slot] == (Int16)CardTypes.Piezo_type)
                {
                    Byte slot_id = (Byte)((slot) + 0x21);
                    byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_PIEZO_SET_ENABLE_PLOT));

                    Byte enable_log_state = Convert.ToByte(cb_piezo_plot_enable.IsChecked);

                    byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot_id, HOST_ID };
                    usb_write("piezo_functions", bytesToSend, 6);

                    cb_piezo_plot_enable.IsChecked = false;
                }
            }




        }

        private void Save_piezo_Plot_Click(object sender, RoutedEventArgs e)
        {
            // stop the timers
            _timer1.Stop();
            timer_piezo_plot.Stop();

            Microsoft.Office.Interop.Excel.Application app = new Microsoft.Office.Interop.Excel.Application();

            string stepper_log_path = Directory.GetCurrentDirectory() + stepper_log_folder_path;
            if (!Directory.Exists(stepper_log_path))
                Directory.CreateDirectory(stepper_log_path);

            try
            {
#if !HYSTERESIS
                app.Visible = false;
                // app.WindowState = XlWindowState.xlMaximized;

                Excel.Workbook wb = app.Workbooks.Add(Excel.XlWBATemplate.xlWBATWorksheet);
                Excel.Worksheet ws = wb.Worksheets[1];
                DateTime currentDate = DateTime.Now;

                string filename = Directory.GetCurrentDirectory() + stepper_log_folder_path + "\\LOG_" + DateTime.Now.ToString("yyyy_MM_dd_HH_mm_ss") + ".xlsx";

                // write the headers in excel
                ws.Range["A1"].Value = currentDate;
                ws.Range["C1"].Value = "Sample Rate:";
                ws.Range["D1"].Value = "250us";
                ws.Range["B3"].Value = "Time (ms)";
                ws.Range["C3"].Value = "Enc Position";
                ws.Range["D3"].Value = "DAC Value";

                // some formating
                Microsoft.Office.Interop.Excel.Range headerColumnRange = ws.get_Range("A1", "D3");
                headerColumnRange.Font.Bold = true;
                // headerColumnRange.Font.Color = 0xFF0000;
                headerColumnRange.EntireColumn.AutoFit();

                // setup the window of data we want to write
                var XRange = SciF_piezo.XAxis.VisibleRange.AsDoubleRange();
                // we multiplied before by log_interval to scale the x axis so now we need to divide
                int minX = (int)(Math.Round(XRange.Min) / .25);
                int maxX = (int)(Math.Round(XRange.Max) / .25);

                if (minX < 0)
                    minX = 0;
                int count = maxX - minX;

                double[,] array = new double[count, 6];

                // copy data into array
                for (int i = 0; i <= count - 1; i++)
                {
                    array[i, 0] = enc_Series.XValues[minX + i];
                    array[i, 1] = enc_Series.YValues[minX + i];
                    array[i, 2] = enc_filtered_Series.YValues[minX + i];
                    array[i, 3] = dac_Series.YValues[minX + i];
                    array[i, 4] = error_Series.YValues[minX + i];
                    array[i, 5] = setpoint_Series.YValues[minX + i];
                }
                // write data to sheet
                var writeRange = ws.Range["B4", "F" + (count + 4)];
                writeRange.Value = array;

                // plot a chart
                object misValue = System.Reflection.Missing.Value;
                Excel.Range chartRange;

                Excel.ChartObjects xlCharts = (Excel.ChartObjects)ws.ChartObjects(Type.Missing);
                Excel.ChartObject myChart = (Excel.ChartObject)xlCharts.Add(320, 10, 600, 250);
                Excel.Chart chartPage = myChart.Chart;

                chartPage.HasTitle = true;
                chartPage.ChartTitle.Text = "Piezo Plot";


                // creates Axis objects for the secondary axes
                //Excel.Axis YAxis2 = (Excel.Axis)chartPage.Axes(Excel.XlAxisType.xlValue, Excel.XlAxisGroup.xlSecondary);

                var yAxis = (Excel.Axis)chartPage.Axes(Excel.XlAxisType.xlValue, Excel.XlAxisGroup.xlPrimary);
                yAxis.HasTitle = true;
                yAxis.AxisTitle.Text = "um";
                yAxis.AxisTitle.Orientation = Excel.XlOrientation.xlUpward;

                Excel.Axis xAxis = (Excel.Axis)chartPage.Axes(Excel.XlAxisType.xlCategory, Excel.XlAxisGroup.xlPrimary);
                xAxis.HasTitle = true;
                xAxis.AxisTitle.Text = "ms";

                chartRange = ws.get_Range("C3", "F" + (count - 2));
                chartPage.SetSourceData(chartRange, misValue);
                chartPage.ChartType = Excel.XlChartType.xlXYScatterLines;

                // enc
                Excel.Range axis_range = ws.get_Range("B4", "B" + (count - 2));
                Excel.Series series = (Excel.Series)chartPage.SeriesCollection(1);
                series.XValues = axis_range;

                // dac
                Excel.Range axis_range2 = ws.get_Range("B4", "B" + (count - 2));
                Excel.Series series2 = (Excel.Series)chartPage.SeriesCollection(2);
                series2.XValues = axis_range2;

                // error
                Excel.Range axis_range3 = ws.get_Range("B4", "B" + (count - 2));
                Excel.Series series3 = (Excel.Series)chartPage.SeriesCollection(3);
                series3.XValues = axis_range3;

                // set the x axis min value to start plot here
                myChart.Chart.Axes(1).MinimumScale = array[0, 0];
                myChart.Chart.Axes(1).MaximumScale = array[count - 2, 0]; ;
                // myChart.Chart.Axes(1).MajorUnit = 5;

                chartPage.SeriesCollection(2).AxisGroup = XlAxisGroup.xlSecondary;
                chartPage.SeriesCollection(3).AxisGroup = XlAxisGroup.xlSecondary;

                wb.SaveAs(filename);

#else
                app.Visible = false;
                // app.WindowState = XlWindowState.xlMaximized;

                Excel.Workbook wb = app.Workbooks.Add(Excel.XlWBATemplate.xlWBATWorksheet);
                Excel.Worksheet ws = wb.Worksheets[1];
                DateTime currentDate = DateTime.Now;

                string filename = Directory.GetCurrentDirectory() + stepper_log_folder_path + "\\LOG_" + DateTime.Now.ToString("yyyy_MM_dd_HH_mm_ss") + ".xlsx";

                // write the headers in excel
                ws.Range["A1"].Value = currentDate;
                ws.Range["C1"].Value = "Sample Rate:";
                ws.Range["D1"].Value = "250us";
                ws.Range["B3"].Value = "Time (ms)";
                ws.Range["C3"].Value = "Enc Position";
                ws.Range["D3"].Value = "DAC Value";

                // some formating
                Microsoft.Office.Interop.Excel.Range headerColumnRange = ws.get_Range("A1", "D3");
                headerColumnRange.Font.Bold = true;
                // headerColumnRange.Font.Color = 0xFF0000;
                headerColumnRange.EntireColumn.AutoFit();

                
                
                // setup the window of data we want to write
                var XRange = SciF_piezo.XAxis.VisibleRange.AsDoubleRange();
                // we multiplied before by log_interval to scale the x axis so now we need to divide
                int minX = (int)(Math.Round(XRange.Min) / .25);
                int maxX = (int)(Math.Round(XRange.Max) / .25);

                if (minX < 0)
                    minX = 0;
                int count = maxX - minX;


                count = enc_Series.Count;


                double[,] array = new double[count, 6];

                // copy data into array
                for (int i = 0; i <= count - 1; i++)
                {
                    array[i, 0] = enc_Series.XValues[minX + i];
                    array[i, 1] = enc_Series.YValues[minX + i];
                    array[i, 2] = enc_filtered_Series.YValues[minX + i];
                    array[i, 3] = dac_Series.YValues[minX + i];
                    array[i, 4] = error_Series.YValues[minX + i];
                    array[i, 5] = setpoint_Series.YValues[minX + i];
                }
                // write data to sheet
                var writeRange = ws.Range["B4", "F" + (count + 4)];
                writeRange.Value = array;

                // plot a chart
                object misValue = System.Reflection.Missing.Value;
                Excel.Range chartRange;

                Excel.ChartObjects xlCharts = (Excel.ChartObjects)ws.ChartObjects(Type.Missing);
                Excel.ChartObject myChart = (Excel.ChartObject)xlCharts.Add(320, 10, 600, 250);
                Excel.Chart chartPage = myChart.Chart;

                chartPage.HasTitle = true;
                chartPage.ChartTitle.Text = "Piezo Plot";


                // creates Axis objects for the secondary axes
                //Excel.Axis YAxis2 = (Excel.Axis)chartPage.Axes(Excel.XlAxisType.xlValue, Excel.XlAxisGroup.xlSecondary);

                var yAxis = (Excel.Axis)chartPage.Axes(Excel.XlAxisType.xlValue, Excel.XlAxisGroup.xlPrimary);
                yAxis.HasTitle = true;
                yAxis.AxisTitle.Text = "um";
                yAxis.AxisTitle.Orientation = Excel.XlOrientation.xlUpward;

                Excel.Axis xAxis = (Excel.Axis)chartPage.Axes(Excel.XlAxisType.xlCategory, Excel.XlAxisGroup.xlPrimary);
                xAxis.HasTitle = true;
                xAxis.AxisTitle.Text = "ms";

                chartRange = ws.get_Range("C3", "F" + (count - 2));
                chartPage.SetSourceData(chartRange, misValue);
                chartPage.ChartType = Excel.XlChartType.xlXYScatterLines;

                // enc
                Excel.Range axis_range = ws.get_Range("B4", "B" + (count - 2));
                Excel.Series series = (Excel.Series)chartPage.SeriesCollection(1);
                series.XValues = axis_range;

                // dac
                Excel.Range axis_range2 = ws.get_Range("B4", "B" + (count - 2));
                Excel.Series series2 = (Excel.Series)chartPage.SeriesCollection(2);
                series2.XValues = axis_range2;

                // error
                Excel.Range axis_range3 = ws.get_Range("B4", "B" + (count - 2));
                Excel.Series series3 = (Excel.Series)chartPage.SeriesCollection(3);
                series3.XValues = axis_range3;

                // set the x axis min value to start plot here
                myChart.Chart.Axes(1).MinimumScale = array[0, 0];
                myChart.Chart.Axes(1).MaximumScale = array[count - 2, 0]; ;
                // myChart.Chart.Axes(1).MajorUnit = 5;

                chartPage.SeriesCollection(2).AxisGroup = XlAxisGroup.xlSecondary;
                chartPage.SeriesCollection(3).AxisGroup = XlAxisGroup.xlSecondary;

                wb.SaveAs(filename);

#endif
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Exception: Fail to write excel file.");
            }
            finally
            {
                if (app != null)
                {
                    app.Quit();
                }
            }

            _timer1.Start();
            timer_piezo_plot.Start();
            clear_focus();
        }





        ///////////////////////////////////////////
        private void btStepper_log_move_by_2_Click(object sender, RoutedEventArgs e)
        {

            Byte slot_number = slot_encoder_on;
            Byte slot_id = (Byte)(slot_number | 0x22);
            slot_id = (Byte)(slot_id | 0x80);
            slot_id += 1;

            float num = float.Parse(tbStepper_log_jog_2.Text);

            string stringValue = num.ToString().Replace(',', '.');
            double value = Convert.ToDouble(stringValue);
            value = (1000 * value) / slot_nm_per_count[slot_number];

            byte[] data1 = BitConverter.GetBytes(Convert.ToInt32(value));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_MOT_MOVE_BY));

            byte[] bytesToSend = new byte[12] { command[0], command[1], 0x06, 0x00, slot_id, HOST_ID,
               slot_number, 0x00,
               data1[0], data1[1], data1[2], data1[3]
            };

            usb_write("piezo_functions", bytesToSend, 12);
            clear_focus();
        }

        private void stepper_goto()
        {
            Byte slot_number = slot_encoder_on;
            Byte slot_id = (Byte)(slot_number | 0x22);
            slot_id = (Byte)(slot_id | 0x80);
            slot_id += 1;

            float num = float.Parse(tbStepper_log_jog_2.Text);


            string stringValue = num.ToString().Replace(',', '.');
            double value = Convert.ToDouble(stringValue);
            // convert back to encoder counts
            value = (1000 * value) / slot_nm_per_count[slot_number];
            value += last_enc_val_cnt;

            byte[] data1 = BitConverter.GetBytes(Convert.ToInt32(value));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_MOVE_ABSOLUTE));

            byte[] bytesToSend = new byte[12] { command[0], command[1], 0x06, 0x00, Convert.ToByte(slot_id | 0x80), HOST_ID,
               slot_number, 0x00,
               data1[0], data1[1], data1[2], data1[3]
            };

            usb_write("piezo_functions", bytesToSend, 12);
        }

        private void btZero_Click(object sender, RoutedEventArgs e)
        {
            zerro_enc_val = last_enc_val_cnt;
            zerro_enc_filtered_val = last_enc_filtered_val_cnt;
        }

        private void btn_piezo_stop_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            // TODO check Byte slot_id = (Byte)(slot_number  | 0x20);
            Byte slot_id = (Byte)((slot_number + 1) | 0x20);
            slot_number -= 1;

            btPiezo_set_mode(slot_id, (byte)Thorlabs.Piezo.Modes.PZ_STOP_MODE);
        }

        private void btn_analog_mode_stop_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            slot_number -= 1;
            Byte slot_id = (Byte)((slot_number + 1) | 0x20);

            btPiezo_set_mode(slot_id, (byte)Thorlabs.Piezo.Modes.PZ_ANALOG_INPUT_MODE);
        }

        public void build_piezo_control(byte _slot)
        {
            this.Dispatcher.Invoke(new Action(() =>
            {
                string slot_name = (_slot + 1).ToString();
                string panel_name = "brd_slot_" + slot_name;
                byte sub_panel_num = 0;
                byte label_num = 0;
                byte button_num = 0;
                byte textbox_num = 0;

                StackPanel sr_sel = (StackPanel)this.FindName(panel_name);

                // Top stack panel
                sub_panel_num = 0;
                cp_subpanels[_slot, sub_panel_num] = new StackPanel();
                cp_subpanels[_slot, sub_panel_num].Orientation = Orientation.Horizontal;
                cp_subpanels[_slot, sub_panel_num].Name = "brd_slot_top_" + slot_name;
                sr_sel.Children.Add(cp_subpanels[_slot, sub_panel_num]);

                // slot label
                Label label = new Label();
                label.Content += "Slot " + slot_name;
                cp_subpanels[_slot, sub_panel_num].Children.Add(label);

                // Piezo label
                label_num = 0;
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Width = 90;
                cp_labels[_slot, label_num].Content += "Piezo";
                cp_subpanels[_slot, sub_panel_num].Children.Add(cp_labels[_slot, label_num]);

                // End Top stack panel

                // 2nd row stack panel
                StackPanel row_2 = new StackPanel();
                row_2.Orientation = Orientation.Horizontal;
                sr_sel.Children.Add(row_2);

                // Stop Button
                button_num = 0;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Stop";
                cp_button[_slot, button_num].Width = 150;
                cp_button[_slot, button_num].Name = "piezo_stop" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_piezo_stop_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_2.Children.Add(cp_button[_slot, button_num]);

                // Analog Input Mode Button
                button_num = 1;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Analog Input Mode";
                cp_button[_slot, button_num].Width = 150;
                cp_button[_slot, button_num].Name = "piezo_analog_mode" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_analog_mode_stop_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_2.Children.Add(cp_button[_slot, button_num]);

                // End 2nd row stack panel

                // 3nd row stack panel
                StackPanel row_3 = new StackPanel();
                row_3.Orientation = Orientation.Horizontal;
                sr_sel.Children.Add(row_3);

                // Put in stop mode label
                label_num = 1;
                cp_labels[_slot, label_num] = new Label();
                cp_labels[_slot, label_num].Width = 900;
                cp_labels[_slot, label_num].Content += "Must not be in analog mode to use jog, press \"Stop\" above";
                row_3.Children.Add(cp_labels[_slot, label_num]);

                // End 3nd row stack panel

                // 4nd row stack panel
                StackPanel row_4 = new StackPanel();
                row_4.Orientation = Orientation.Horizontal;
                sr_sel.Children.Add(row_4);

                // Jog Down Button
                button_num = 2;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Jog Down";
                cp_button[_slot, button_num].Width = 60;
                cp_button[_slot, button_num].Name = "pz_jog_down" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btnJogDown_Click);
                cp_button[_slot, button_num].Margin = new Thickness(10, 5, 5, 5);
                row_4.Children.Add(cp_button[_slot, button_num]);

                // Jog Up Button
                button_num = 3;
                cp_button[_slot, button_num] = new Button();
                cp_button[_slot, button_num].Content = "Jog up";
                cp_button[_slot, button_num].Width = 60;
                cp_button[_slot, button_num].Name = "pz_jog_up" + slot_name;
                cp_button[_slot, button_num].Click += new RoutedEventHandler(btnJogUp_Click);
                cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                row_4.Children.Add(cp_button[_slot, button_num]);

                // Jog textbox
                textbox_num = 0;
                cp_textbox[_slot, textbox_num] = new TextBox();
                cp_textbox[_slot, textbox_num].Text = "10";
                cp_textbox[_slot, textbox_num].Width = 100;
                cp_textbox[_slot, textbox_num].Margin = new Thickness(5, 5, 5, 5);
                row_4.Children.Add(cp_textbox[_slot, textbox_num]);

                // End 4th row stack panel
            }));
        }

        private void btnJogDown_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            slot_number -= 1;
            // TODO check Byte slot_id = (Byte)(slot_number  | 0x20);
            Byte slot_id = (Byte)((slot_number + 1) | 0x20);
            

            byte chan_id = 6; // piezo always on slot 7
            byte mode = 0;
            float num = float.Parse(cp_textbox[slot_number, 0].Text);
            if (num > 100)
            {
                MessageBox.Show("Value must be between 0 and 100", "Error");
                return;
            }
            string stringValue = num.ToString().Replace(',', '.');
            double value = Convert.ToDouble(stringValue);
            value = (1000 * value) / slot_nm_per_count[slot_encoder_on];
            value *= -1;

            byte[] data1 = BitConverter.GetBytes(Convert.ToInt32(value));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_PIEZO_SET_MOVE_BY));

            byte[] bytesToSend = new byte[6 + 6] { command[0], command[1], 6, 0x00, (Convert.ToByte(slot_id  | 0x80)), HOST_ID,
                chan_id, mode,          // 6-7
                data1[0], data1[1], data1[2], data1[3]
                };

            usb_write("piezo_functions", bytesToSend, 6 + 6);
            Thread.Sleep(20);
        }

        private void btnJogUp_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            slot_number -= 1;
            // TODO check Byte slot_id = (Byte)(slot_number  | 0x20);
            Byte slot_id = (Byte)((slot_number + 1) | 0x20);


            byte chan_id = 6; // piezo always on slot 7
            byte mode = 0;
            float num = float.Parse(cp_textbox[slot_number, 0].Text);
            if (num > 100)
            {
                MessageBox.Show("Value must be between 0 and 100", "Error");
                return;
            }
            string stringValue = num.ToString().Replace(',', '.');
            double value = Convert.ToDouble(stringValue);
            value = (1000 * value) / slot_nm_per_count[slot_encoder_on];

            byte[] data1 = BitConverter.GetBytes(Convert.ToInt32(value));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_PIEZO_SET_MOVE_BY));

            byte[] bytesToSend = new byte[6 + 6] { command[0], command[1], 6, 0x00, (Convert.ToByte(slot_id  | 0x80)), HOST_ID,
                chan_id, mode,          // 6-7
                data1[0], data1[1], data1[2], data1[3]
                };

            usb_write("piezo_functions", bytesToSend, 6 + 6);
            Thread.Sleep(20);
        }
    }
}