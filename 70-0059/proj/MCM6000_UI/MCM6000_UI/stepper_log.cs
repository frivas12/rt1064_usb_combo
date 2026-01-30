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


using Microsoft.Office.Interop.Excel;
using Excel = Microsoft.Office.Interop.Excel;
using Button = System.Windows.Controls.Button;

using System.Windows.Threading; // for plot timer
using System.Collections.ObjectModel;

namespace MCM6000_UI
{
    public partial class MainWindow
    {
        // private XyDataSeries<double, double> _dataSeries;
        // static System.Timers.Timer _timer_plot;
        static DispatcherTimer timer_plot;
        // private Random _random;
        //   private DateTime? _startTime;

        private double _windowSize = 10000;
        //private double _timeNow = 0;
        private bool _showLatestWindow = true;
        private bool _thatWasADoubleClick;

        public int last_slot_selected = -1;
        public double log_interval = 10;
        public double data_count = 0;

        Double command_pos, enc;
        bool thermal_warn, thermal_shutdown;

        public struct SlotData
        {
            public XyDataSeries<double, double> cmd_pos_Series;
            public XyDataSeries<double, double> enc_count_Series;
            public XyDataSeries<double, int> thermal_warn_series;
            public XyDataSeries<double, int> thermal_shutdown_series;
            public XyDataSeries<double, int> step_loss_series;
            public XyDataSeries<double, int> ocd_series;
         //   public XyDataSeries<double, double> scroll_Series;
            public double ms_x_axis;
            public bool plot_enable;
        };
        SlotData slot_data = new SlotData();

#if false      // used to hide autoscroll legend
        public class FilteringLegendModifier : LegendModifier
        {
            public static bool GetIncludeSeries(DependencyObject obj)
            {
                return (bool)obj.GetValue(IncludeSeriesProperty);
            }

            public static void SetIncludeSeries(DependencyObject obj, bool value)
            {
                obj.SetValue(IncludeSeriesProperty, value);
            }

            // Using a DependencyProperty as the backing store for IncludeSeries.  This enables animation, styling, binding, etc...
            public static readonly DependencyProperty IncludeSeriesProperty = DependencyProperty.RegisterAttached("IncludeSeries", typeof(bool), typeof(FilteringLegendModifier), new PropertyMetadata(true));

            protected override ObservableCollection<SeriesInfo> GetSeriesInfo(IEnumerable<IRenderableSeries> allSeries)
            {
                var filteredSeries = allSeries.Where(s => s is UIElement)
                                                                       .Where(s => (bool)((UIElement)s).GetValue(IncludeSeriesProperty));
                return base.GetSeriesInfo(filteredSeries);
            }
        }
#endif
        private void initialize_data_series_plots()
        {
            // Command postion
            slot_data.cmd_pos_Series = new XyDataSeries<double, double>();
            slot_data.ms_x_axis = 0;
            LineSeries_cmd_pos_Series.DataSeries = slot_data.cmd_pos_Series;
            LineSeries_cmd_pos_Series.SeriesColor = Colors.Yellow;
            slot_data.cmd_pos_Series.SeriesName = "Command Pos";

            // Encoder postion
            slot_data.enc_count_Series = new XyDataSeries<double, double>();
            //slot_data.pos_updater = 0;
            LineSeries_enc_pos_Series.DataSeries = slot_data.enc_count_Series;
            LineSeries_enc_pos_Series.SeriesColor = Colors.Green;
            slot_data.enc_count_Series.SeriesName = "Enc Pos";

#if ENABLE_OVERHEAT_LOGGING
            // Thermal warning
            slot_data.thermal_warn_series = new();
            LineSeries_thermal_warn_Series.DataSeries = slot_data.thermal_warn_series;
            LineSeries_thermal_warn_Series.SeriesColor = Colors.Purple;
            slot_data.thermal_warn_series.SeriesName = "Thermal Warning";

            // Thermal shutdown
            slot_data.thermal_shutdown_series = new();
            LineSeries_thermal_shutdown_Series.DataSeries = slot_data.thermal_shutdown_series;
            LineSeries_thermal_shutdown_Series.SeriesColor = Colors.Red;
            slot_data.thermal_shutdown_series.SeriesName = "Thermal Shutdown";

            // Step Loss
            slot_data.step_loss_series = new();
            LineSeries_step_loss_Series.DataSeries = slot_data.step_loss_series;
            LineSeries_step_loss_Series.SeriesColor = Colors.Blue;
            slot_data.step_loss_series.SeriesName = "Step Loss";

            // OCD
            slot_data.ocd_series = new();
            LineSeries_ocd_Series.DataSeries = slot_data.ocd_series;
            LineSeries_ocd_Series.SeriesColor = Colors.Aqua;
            slot_data.ocd_series.SeriesName = "Overcurrent";
#endif

            slot_data.plot_enable = false;

            // Setup autoscrolling
            // from https://support.scichart.com/index.php?/Knowledgebase/Article/View/17260/40/how-to-create-a-scrolling-strip-chart-in-scichart 
            // (2): Create a DataSeries and assign to FastLineRenderableSeries
            // _dataSeries = new XyDataSeries<double, double>();
            // _random = new Random();
            // lineSeries.DataSeries = _dataSeries;

            // scroll_Series
           // slot_data.scroll_Series = new XyDataSeries<double, double>();
           // LineSeries_scroll_Series.DataSeries = slot_data.enc_count_Series;
           // LineSeries_scroll_Series.SeriesColor = Colors.White;
           // slot_data.scroll_Series.SeriesName = "Scroll";



            // (6): We subscribe to PreviewMouseDown to set a flag to prevent scrolling calculation in (5)
            SciF.PreviewMouseDown += (s, arg) =>
            {
                // On mouse down (but not double click), freeze our last N seconds window 
                if (!_thatWasADoubleClick) _showLatestWindow = false;

                _thatWasADoubleClick = false;
            };

            // (7): Subscribe to PreviewMouseDoubleClick to re-enable the auto scrolling window
            SciF.PreviewMouseDoubleClick += (s, arg) =>
            {
                _showLatestWindow = true;
                _thatWasADoubleClick = true; // (8): Prevent contention between double click and single click event

                // Decouple the instance so you don't modify original
                var currentRange = (DoubleRange)SciF.YAxes[0].VisibleRange.AsDoubleRange().Clone();
                //var currentRange = (DoubleRange)SciF.YAxis.VisibleRange.AsDoubleRange().Clone();
                /*  if (value > currentRange.Max)
                  {
                      // Expand range
                      currentRange.Max = value;

                      // Apply growby
                      currentRange = currentRange.GrowBy(SciF.YAxis.GrowBy);

                      SciF.YAxis.VisibleRange.SetMinMax(0, currentRange.Max);
                  }*/
                double min = currentRange.Min - (currentRange.Min * .1);
                double max = currentRange.Max + (currentRange.Max * .1);
                if (min == 0)
                    min = -20;
                if (max == 0)
                    max = +20;

                // Restore our last N seconds window on double click
                SciF.YAxes[0].AnimateVisibleRangeTo(new DoubleRange(min, max), TimeSpan.FromMilliseconds(200));
                SciF.XAxis.AnimateVisibleRangeTo(new DoubleRange(slot_data.ms_x_axis - _windowSize, slot_data.ms_x_axis), TimeSpan.FromMilliseconds(200));
            };

            // (3): Create a timer to tick new data 
            timer_plot = new DispatcherTimer(DispatcherPriority.Render);
            timer_plot.Interval = TimeSpan.FromSeconds(1);
            timer_plot.Tick += TimerOnTick;
            timer_plot.Start();

#if false
            // In MainWindow.OnLoaded, add this code
            _labelProvider = new RelativeTimeLabelProvider();
            xAxis.LabelProvider = _labelProvider;
            SciF.InvalidateElement();
#endif
            _windowSize = sl_plot_scroll_size.Value;
           //  LegendModifier.SetIncludeSeries(LineSeries_scroll_Series, false);

        }

        private void TimerOnTick(object sender, EventArgs eventArgs)
        {
#if true
            if (last_slot_selected == -1)
            {
                return;
            }

#if true
          //  _timeNow++;

            // (4): Append next sample            
          //  slot_data.scroll_Series.Append(slot_data.ms_x_axis, enc);

            // (5): Update visible range if we are in the mode to show the latest window of N seconds
            if (_showLatestWindow)
            {
                xAxis.AnimateVisibleRangeTo(new DoubleRange(slot_data.ms_x_axis - _windowSize, slot_data.ms_x_axis), TimeSpan.FromMilliseconds(280));
            }
#else
            _timeNow++;

            // Append next sample            
            slot_data.scroll_Series.Append(_timeNow, enc);

            // Update currentTime in LabelProvider to calculate relative labels
            _labelProvider.CurrentTime = _timeNow;

            // Update visible range if we are in the mode to show the latest window of N seconds
            if (_showLatestWindow)
            {
                xAxis.AnimateVisibleRangeTo(new DoubleRange(_timeNow - _windowSize, _timeNow), TimeSpan.FromMilliseconds(280));
            }
#endif
#endif
        }

        public class RelativeTimeLabelProvider : NumericLabelProvider
        {
            // Update me with current latest time every time you append data!
            public double CurrentTime { get; set; }

            public override string FormatCursorLabel(IComparable dataValue)
            {
                // dataValue is the actual axis label value, e.g. comes from DataSeries.XValues
                var value = (double)dataValue;
                var relative = (CurrentTime - value);

                return $"t-{relative:0.0}";
            }

            public override string FormatLabel(IComparable dataValue)
            {
                // dataValue is the actual axis label value, e.g. comes from DataSeries.XValues
                var value = (double)dataValue;
                var relative = (CurrentTime - value);

                return $"t-{relative:0.0}";
            }
        }

        /* Stepper log update on _timer1_Elapsed*/
        public void stepper_log_request(byte _slot)
        {
            Byte slot_id = (Byte)((_slot + 1) | 0x20);
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_STEPPER_LOG));

            byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot_id, HOST_ID };
            usb_write("stepper_log", bytesToSend, 6);
        }

        public void plot_slot_values()
        {
            if (last_slot_selected == -1)
            {

            }
            else
            {
                Byte slot_id = (Byte)((last_slot_selected + 1) | 0x20);
                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_STEPPER_LOG));

                byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot_id, HOST_ID };
                usb_write("stepper_log", bytesToSend, 6);

            }
        }

        // return from request MGMSG_MCM_REQ_STEPPER_LOG
        public void stepper_log_update()
        {
            byte _slot = ext_data[0];

            // command position
            command_pos = ext_data[2] | ext_data[3] << 8 | ext_data[4] << 16 | ext_data[5] << 24;
            command_pos = command_pos * slot_nm_per_count[_slot] / 1e3;

            // encoder counts 
            enc = ext_data[10] | ext_data[11] << 8 | ext_data[12] << 16 | ext_data[13] << 24;
            enc = enc * slot_nm_per_count[_slot] / 1e3;

            //    pid_error += enc;
            if (ext_data.Length > 0x0E)
            {
                ushort stepper_status_bits = BitConverter.ToUInt16(ext_data, 14);
                thermal_warn = (stepper_status_bits & (1 << 10)) == 0;
                thermal_shutdown = (stepper_status_bits & (1 << 11)) == 0;

                bool step_loss = (stepper_status_bits & (3 << 13)) != 3 << 13;
                bool ocd = (stepper_status_bits & (1 << 12)) == 0;
            }

#if false
            // step counts 
            command_pos = ext_data[2] | ext_data[3] << 8 | ext_data[4] << 16 | ext_data[5] << 24;
            double nm_per_step = (slot_nm_per_count[_slot] / (slot_counts_per_unit[_slot]) * 100000);
            command_pos = command_pos * nm_per_step / 1e3;

            command_pos -= enc;
#endif


#if true
            slot_data.cmd_pos_Series.Append(slot_data.ms_x_axis, command_pos);
            slot_data.cmd_pos_Series.Update(slot_data.ms_x_axis, command_pos);
            slot_data.enc_count_Series.Append(slot_data.ms_x_axis, enc);
            slot_data.enc_count_Series.Update(slot_data.ms_x_axis, enc);
#if ENABLE_OVERHEAT_LOGGING
            slot_data.thermal_warn_series.Append(slot_data.ms_x_axis, thermal_warn ? 1 : 0);
            slot_data.thermal_warn_series.Update(slot_data.ms_x_axis, thermal_warn ? 1 : 0);
            slot_data.thermal_shutdown_series.Append(slot_data.ms_x_axis, thermal_shutdown ? 1 : 0);
            slot_data.thermal_shutdown_series.Update(slot_data.ms_x_axis, thermal_shutdown ? 1 : 0);
            slot_data.step_loss_series.Append(slot_data.ms_x_axis, step_loss ? 1 : 0);
            slot_data.step_loss_series.Update(slot_data.ms_x_axis, step_loss ? 1 : 0);
            slot_data.ocd_series.Append(slot_data.ms_x_axis, ocd ? 1 : 0);
            slot_data.ocd_series.Update(slot_data.ms_x_axis, ocd ? 1 : 0);
#endif

#else // used to see how many bytes are in the USB receive buffer 
            slot_data.cmd_pos_Series.Append(slot_data.ms_x_axis, 0);
            slot_data.cmd_pos_Series.Update(slot_data.ms_x_axis, 0);
            slot_data.enc_count_Series.Append(slot_data.ms_x_axis, usb_bytes);
            slot_data.enc_count_Series.Update(slot_data.ms_x_axis, usb_bytes);
#endif

            slot_data.ms_x_axis += log_interval;
            data_count++;
#if false
            if (last_slot_selected == -1)
            {
                return;
            }

            _timeNow++;

            // (4): Append next sample            
            //_dataSeries.Append(_timeNow, _random.NextDouble());
            slot_data.scroll_Series.Append(_timeNow, enc);

            // (5): Update visible range if we are in the mode to show the latest window of N seconds
            if (_showLatestWindow)
            {
                xAxis.AnimateVisibleRangeTo(new DoubleRange(_timeNow - _windowSize, _timeNow), TimeSpan.FromMilliseconds(280));
            }
#endif

        }


        private void btnLog_Goto_Click(object sender, RoutedEventArgs e)
        {
            if (log_slot_select.SelectedIndex == -1)
                return;

            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = Convert.ToString(log_slot_select.SelectedIndex + 1);

            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            float num = float.Parse(tbStepper_log_goto_1.Text);

            string stringValue = num.ToString().Replace(',', '.');
            double value = Convert.ToDouble(stringValue);
            value = (1000 * value) / slot_nm_per_count[slot_number];

            byte[] data1 = BitConverter.GetBytes(Convert.ToInt32(value));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_MOVE_ABSOLUTE));

            byte[] bytesToSend = new byte[12] { command[0], command[1], 0x06, 0x00, Convert.ToByte(slot_id | 0x80), HOST_ID,
               slot_number, 0x00,
               data1[0], data1[1], data1[2], data1[3]
            };

            usb_write("stepper_log", bytesToSend, 12);
            clear_focus();
        }
        private void btnLog_move_by_Click(object sender, RoutedEventArgs e)
        {
            if (log_slot_select.SelectedIndex == -1)
                return;

            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = Convert.ToString(log_slot_select.SelectedIndex + 1);

            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            float num = float.Parse(tbStepper_log_jog_1.Text);

            string stringValue = num.ToString().Replace(',', '.');
            double value = Convert.ToDouble(stringValue);
            value = (1000 * value) / slot_nm_per_count[slot_number];

            byte[] data1 = BitConverter.GetBytes(Convert.ToInt32(value));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_MOT_MOVE_BY));

            byte[] bytesToSend = new byte[12] { command[0], command[1], 0x06, 0x00, Convert.ToByte(slot_id | 0x80), HOST_ID,
               slot_number, 0x00,
               data1[0], data1[1], data1[2], data1[3]
            };

            usb_write("stepper_log", bytesToSend, 12);
            clear_focus();
        }

        private void log_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (log_slot_select.SelectedIndex == -1)
                return;
            else
            {
                Clear_Plot_Click(sender, e);

                // get the stage selected from the combo  box 
                byte slot = (byte)(Convert.ToByte(log_slot_select.SelectedIndex) + 0x21);

                byte slot_num = (byte)(Convert.ToByte(log_slot_select.SelectedIndex));

                // request PID params
                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_REQ_DCPIDPARAMS));
                byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
                usb_write("stepper_log", bytesToSend, 6);
                Thread.Sleep(20);

                switch (log_slot_select.SelectedIndex)
                {
                    case 0:
                        last_slot_selected = SLOT1;
                        break;

                    case 1:
                        last_slot_selected = SLOT2;
                        break;

                    case 2:
                        last_slot_selected = SLOT3;
                        break;

                    case 3:
                        last_slot_selected = SLOT4;
                        break;

                    case 4:
                        last_slot_selected = SLOT5;
                        break;

                    case 5:
                        last_slot_selected = SLOT6;
                        break;

                    case 6:
                        last_slot_selected = SLOT7;
                        break;

                    case 7:
                        last_slot_selected = SLOT8;
                        break;

                }
            }
        }

        private void Clear_Plot_Click(object sender, RoutedEventArgs e)
        {
            if (log_slot_select.SelectedIndex == -1)
            {
                clear_focus();
                return;
            }

            else
            {
                slot_data.cmd_pos_Series.Clear();
                slot_data.enc_count_Series.Clear();
              //  slot_data.scroll_Series.Clear();
                slot_data.ms_x_axis = 0;
                slot_data.plot_enable = false;
                data_count = 0;
                clear_focus();
            }
        }

        private void bt_params_refresh_Click(object sender, RoutedEventArgs e)
        {
            log_SelectionChanged(sender, null);
            clear_focus();
        }

        private void save_stepper_pid_params_s_Click(object sender, RoutedEventArgs e)
        {
            if (log_slot_select.SelectedIndex == -1)
                return;

            // get the stage selected from the combo  box 
            byte slot_id = (byte)((Convert.ToByte(log_slot_select.SelectedIndex) + 0x21) | 0x80);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_SET_DCPIDPARAMS));
            byte[] Kp = BitConverter.GetBytes(Convert.ToInt32(tb_p_term_s.Text));
            byte[] Ki = BitConverter.GetBytes(Convert.ToInt32(tb_i_term_s.Text));
            byte[] Kd = BitConverter.GetBytes(Convert.ToInt32(tb_d_term_s.Text));
            byte[] iMax = BitConverter.GetBytes(Convert.ToInt32(tb_imax_s.Text));
            byte[] bytesToSend;

            bytesToSend = new byte[26] { command[0], command[1], 20, 0, Convert.ToByte(slot_id | 0x80), HOST_ID,
            (byte)(pid_slot - 1), 0x00,
            Kp[0], Kp[1], Kp[2], Kp[3],
            Ki[0], Ki[1], Ki[2], Ki[3],
            Kd[0], Kd[1], Kd[2], Kd[3],
            iMax[0], iMax[1], iMax[2], iMax[3],
            0x00, 0x00};

            usb_write("stepper_log", bytesToSend, 26);
            clear_focus();
        }

        private void btn_goto_stored_position_log_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;

            if (log_slot_select.SelectedIndex == -1)
                return;

            // get the stage selected from the combo  box 
            byte slot_id = (byte)(Convert.ToByte(log_slot_select.SelectedIndex) + 0x21);
            Byte slot_number = (byte)(Convert.ToByte(log_slot_select.SelectedIndex));


            // get the position number
            string pos_num_t = button_name.Substring(button_name.Length - 1, 1);
            Byte pos_num = Convert.ToByte(pos_num_t);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_SET_GOTO_STORE_POSITION));

            byte[] bytesToSend = new byte[6] { command[0], command[1], slot_number, pos_num, slot_id, HOST_ID };

            usb_write("stepper_log", bytesToSend, 6);
            clear_focus();
        }

        private void Save_Plot_Click(object sender, RoutedEventArgs e)
        {
            // stop the timers
            _timer1.Stop();
            timer_plot.Stop();

            Microsoft.Office.Interop.Excel.Application app = new Microsoft.Office.Interop.Excel.Application();

            string stepper_log_path = Directory.GetCurrentDirectory() + stepper_log_folder_path;
            if (!Directory.Exists(stepper_log_path))
                Directory.CreateDirectory(stepper_log_path);

            try
            {
                app.Visible = false;
               // app.WindowState = XlWindowState.xlMaximized;

                Workbook wb = app.Workbooks.Add(XlWBATemplate.xlWBATWorksheet);
                Worksheet ws = wb.Worksheets[1];
                DateTime currentDate = DateTime.Now;

                string filename = Directory.GetCurrentDirectory() + stepper_log_folder_path +  "\\LOG_" + DateTime.Now.ToString("yyyy_MM_dd_HH_mm_ss") + ".xlsx";

                // write the headers in excel
                ws.Range["A1"].Value = currentDate;
                ws.Range["C1"].Value = "Sample Rate:";
                ws.Range["D1"].Value = log_interval;
                ws.Range["B3"].Value = "Time (ms)";
                ws.Range["C3"].Value = "Command Position";
                ws.Range["D3"].Value = "Encoder Position";

                // some formating
                Microsoft.Office.Interop.Excel.Range headerColumnRange = ws.get_Range("A1", "D3");
                headerColumnRange.Font.Bold = true;
               // headerColumnRange.Font.Color = 0xFF0000;
                headerColumnRange.EntireColumn.AutoFit();

                // setup the window of data we want to write
                var XRange = SciF.XAxis.VisibleRange.AsDoubleRange();
                // we multiplied before by log_interval to scale the x axis so now we need to divide
                int minX = (int)(Math.Round(XRange.Min) / log_interval); 
                int maxX = (int)(Math.Round(XRange.Max) / log_interval);

                if (minX < 0)
                    minX = 0;
                int count = maxX - minX;
                
                double[,] array = new double[count, 3];

                // copy data into array
                for (int i = 0; i <= count - 1; i++)
                {
                    array[i,0] = slot_data.cmd_pos_Series.XValues[minX + i];
                    array[i, 1] = slot_data.cmd_pos_Series.YValues[minX + i];
                    array[i, 2] = slot_data.enc_count_Series.YValues[minX + i];
                }
                // write data to sheet
                var writeRange = ws.Range["B4", "D" + (count + 4)];
                writeRange.Value = array;

                // plot a chart
                object misValue = System.Reflection.Missing.Value;
                Excel.Range chartRange;

                Excel.ChartObjects xlCharts = (Excel.ChartObjects)ws.ChartObjects(Type.Missing);
                Excel.ChartObject myChart = (Excel.ChartObject)xlCharts.Add(320, 10, 600, 250);
                Excel.Chart chartPage = myChart.Chart;
                
                chartPage.HasTitle = true;
                chartPage.ChartTitle.Text = "Position Plot";
                
                var yAxis = (Excel.Axis)chartPage.Axes(Excel.XlAxisType.xlValue, Excel.XlAxisGroup.xlPrimary);
                yAxis.HasTitle = true;
                yAxis.AxisTitle.Text = "um";
                yAxis.AxisTitle.Orientation = Excel.XlOrientation.xlUpward;

                Excel.Axis xAxis = (Excel.Axis)chartPage.Axes(Excel.XlAxisType.xlCategory, Excel.XlAxisGroup.xlPrimary);
                xAxis.HasTitle = true;
                xAxis.AxisTitle.Text = "ms";

                chartRange = ws.get_Range("C3", "D" + (count - 2));
                chartPage.SetSourceData(chartRange, misValue);
                chartPage.ChartType = Excel.XlChartType.xlXYScatterLines;

                Excel.Range axis_range = ws.get_Range("B4", "B" + (count - 2));
                Excel.Series series = (Excel.Series)chartPage.SeriesCollection(1);
                series.XValues = axis_range;
                

                Excel.Range axis_range2 = ws.get_Range("B4", "B" + (count - 2));
                Excel.Series series2 = (Excel.Series)chartPage.SeriesCollection(2);
                series2.XValues = axis_range2;

               // set the x axis min value to start plot here
                myChart.Chart.Axes(1).MinimumScale = array[0,0];
                 myChart.Chart.Axes(1).MaximumScale = array[count - 2, 0];;
                // myChart.Chart.Axes(1).MajorUnit = 5;

                wb.SaveAs(filename);
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
            timer_plot.Start();
            clear_focus();
        }

        private void tb_log_interval_TextChanged(object sender, TextChangedEventArgs e)
        {
            log_interval = Convert.ToDouble(tb_log_interval.Text);
        }

        private void sl_plot_scroll_size_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            _windowSize = sl_plot_scroll_size.Value;
        }

    }
}