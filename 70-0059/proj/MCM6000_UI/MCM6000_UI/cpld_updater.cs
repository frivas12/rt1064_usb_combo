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
using System.Text.RegularExpressions;
using System.Runtime.InteropServices;
using System.Diagnostics;
namespace MCM6000_UI
{
    public partial class MainWindow
    {
        private void btOpen_cpld_file_Click(object sender, RoutedEventArgs e)
        {
            if (!_programmingPath.ProvidesCPLD)
            { return; }

            loading_cpld = false;
            lbl_con_status.Dispatcher.Invoke(new Action(() =>
            {
                // disable all tabs and controls
                MCM6000_tabs.IsEnabled = false;
                lbl_con_status.Content = "Oepning JED file..";
                pbar_cpld.Visibility = Visibility.Visible;
                lbl1pbar_cpld.Visibility = Visibility.Visible;
            }));

            _worker = new BackgroundWorker();
            _worker.WorkerReportsProgress = true;
            _worker.RunWorkerCompleted += _worker_RunWorkerCompleted_CPLD;
            _worker.ProgressChanged += _worker_ProgressChanged_CPLD;
            _worker.DoWork += (sender, e) =>
            {
                loading_cpld = true;
                _timer1.Enabled = false;
                using (var stream = _programmingPath.OpenCPLD())
                {
                    update_CPLD(stream);
                }

                UpdateProgress(100); // -1 is the index used to update the status the label
                Thread.Sleep(1500);
            };
            _worker.RunWorkerAsync();
        }

        void update_CPLD(Stream stream)
        {
            int dbg1 = 0;
            double prt = 0;
            double remaining = 0;
            Int16 i, j, k;
            string new_line;
            Int32 fuse_count;  // this is the total real fuse count = lines * 128bits
            string[] new_lines = new string[8000];
            byte[] data_bytes = new byte[16];
            Int16 line_count = 0;
            string temp;
            byte[,] lines_in_hex_format = new byte[8000, 16];
            string lines = "CPLD LINES\r\n";

            const string PROFILE = "cpld-update";
            SetProfileLock(PROFILE);
            var cooldownBackup = _txScheduler.CooldownMs;
            _txScheduler.CooldownMs = 1;
            try
            {
                System.IO.StreamReader file = new StreamReader(stream);

                // read file until we reach the QF
                // The key word QF identifies the total real fuse count of the device
                new_line = file.ReadLine();
                do
                {
                    new_line = file.ReadLine();
                } while (!new_line.StartsWith("QF"));

                new_line = new_line.Substring(2, new_line.Length - 3); // remove the QF and the '*' at the end
                fuse_count = Convert.ToInt32(new_line);
                // ------------------------------------------------------------------ 
                UpdateProgress((int)(prt += 3));
                Thread.Sleep(300);
                // goto the L000000 line
                new_line = file.ReadLine();
                do
                {
                    new_line = file.ReadLine();
                } while (!new_line.StartsWith("L"));
                // ------------------------------------------------------------------ 
                UpdateProgress((int)(prt += 3));
                Thread.Sleep(300);
                // this is where the configuration data starts
                // read to the NOTE '*' just before next line "END CONFIG DATA* " and count how many lines we are sending
                do
                {
                    new_lines[line_count++] = file.ReadLine();
                } while (!new_lines[line_count - 1].StartsWith("*"));
                line_count--;
                // ------------------------------------------------------------------ 
                UpdateProgress((int)(prt += 3));
                Thread.Sleep(300);

                // send command to start programing 
                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_CPLD_UPDATE));

                byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, MOTHERBOARD_ID, HOST_ID };
                usb_write(PROFILE, bytesToSend, 6);
                
                // wait for response to let us know controler is ready
                while (cpld_data_recieved_flag == 0) { Thread.Sleep(1); }
                cpld_data_recieved_flag = 0;

                if (cpld_data_recieved == 1)
                {
                    // error
                    lbl_con_status.Dispatcher.Invoke(new Action(() =>
                    { lbl_con_status.Content = "Problem updating CPLD flash. Error 1"; }));
                    MessageBox.Show("Problem updating CPLD flash. Error 1");
                    //start_update_gui(sender, e);
                    //_timer1.Start();
                    return;
                }
                // CONFIGURATION_FLASH_START
                lbl_con_status.Dispatcher.Invoke(new Action(() =>
                { lbl_con_status.Content = "Flash Configuration"; }));
                // send the number of lines for the configuration flash
                byte[] data = BitConverter.GetBytes(line_count);
                //            write_usb(data, 2);
                usb_write(PROFILE, data, 2);

                // wait for response to let us know controler is ready

                while (cpld_data_recieved_flag == 0) { };

                cpld_data_recieved_flag = 0;
                if (cpld_data_recieved == 1)
                {
                    // error
                    lbl_con_status.Dispatcher.Invoke(new Action(() =>
                    { lbl_con_status.Content = "Problem updating CPLD flash. Error 2"; }));
                    MessageBox.Show("Problem updating CPLD flash. Error 2");
                    //start_update_gui(sender, e);
                    //_timer1.Start();
                    return;
                }
                // CONFIGURATION_FLASH_PROGRAM
                // send line by line of the configuration flash

                lbl_con_status.Dispatcher.Invoke(new Action(() =>
                { lbl_con_status.Content = "Loading Configuration Flash. "; }));

                //After following for loop, progress is upto 80%
                //So increase progress bar with each line for

                int dotj = 0;
                for (i = 0; i < line_count; i++)
                {
                    if ((i % 100) == 0)
                    {
                        dotj++;

                        if (dotj % 3 == 1)
                            lbl_con_status.Dispatcher.Invoke(new Action(() =>
                            { lbl_con_status.Content = "Loading Configuration Flash. "; }));
                        if (dotj % 3 == 2)
                            lbl_con_status.Dispatcher.Invoke(new Action(() =>
                            { lbl_con_status.Content = "Loading Configuration Flash.. "; }));
                        if (dotj % 3 == 0)
                            lbl_con_status.Dispatcher.Invoke(new Action(() =>
                            { lbl_con_status.Content = "Loading Configuration Flash... "; }));
                        if (line_count > 4400)
                            UpdateProgress((int)(prt += 1));
                        else
                            UpdateProgress((int)(prt += 2));
                        Thread.Sleep(30);
                    }
                    lines += i.ToString();
                    lines += "- ";
                    k = 0;
                    // convert line string to binary
                    for (j = 0; j < 16; j++)
                    {
                        temp = new_lines[i].Substring(k, 8);
                        data_bytes[j] = Convert.ToByte(temp, 2);
                        lines_in_hex_format[i, j] = data_bytes[j];
                        k += 8;
                        lines += data_bytes[j].ToString("D3");
                        lines += " ";
                    }
                    lines += "\r\n";
                    usb_write(PROFILE, data_bytes, 16);
                    // wait for response to let us know controler is ready
                    //                System.Threading.Thread.Sleep(900);
                    // wait for response to let us know controler is ready
                    while (cpld_data_recieved_flag == 0) { };
                    cpld_data_recieved_flag = 0;
                    //  data_back = read_usb_prog(6);

                    if (cpld_data_recieved == 1)
                    {
                        // error
                        lbl_con_status.Dispatcher.Invoke(new Action(() =>
                        { lbl_con_status.Content = "Problem updating CPLD flash.  Try again. Error 3"; }));
                        MessageBox.Show("Problem updating CPLD flash.  Try again. Error 3");
                        //start_update_gui(sender, e);
                        // _timer1.Start();
                        return;
                    }

                }

#if false
                System.IO.StreamWriter filelines = new System.IO.StreamWriter(@"CPLD Lines.txt", false);
                filelines.WriteLine(lines);
                filelines.Close();
#endif
                // WRITE_FEATURE_ROW
                lbl_con_status.Dispatcher.Invoke(new Action(() =>
                { lbl_con_status.Content = "Loading Feature Row in Flash..        "; }));
                Thread.Sleep(30);
                do
                {
                    new_lines[0] = file.ReadLine();
                } while (!new_lines[0].StartsWith("E"));

                //Progress = 81
                // convert line string to binary
                k = 1;
                remaining = 100 - prt;
                for (j = 0; j < 8; j++)
                {
                    UpdateProgress((int)(prt += ((remaining / 9) - 1)));
                    Thread.Sleep(40);
                    temp = new_lines[0].Substring(k, 8);
                    data_bytes[j] = Convert.ToByte(temp, 2);
                    k += 8;
                }
                //            progressBar1.Hide();

                usb_write(PROFILE, data_bytes, 8);

                // wait for response to let us know controler is ready
                while (cpld_data_recieved_flag == 0) { };
                cpld_data_recieved_flag = 0;
                //  data_back = read_usb_prog(6);

                if (cpld_data_recieved == 1)
                {
                    // error
                    lbl_con_status.Dispatcher.Invoke(new Action(() =>
                    { lbl_con_status.Content = "Problem updating CPLD flash.  Try again. Error 4"; }));
                    MessageBox.Show("Problem updating CPLD flash.  Try again. Error 4");
                    //start_update_gui(sender, e);
                    //_timer1.Start();
                    return;
                }
                // WRITE_FEABITS
                lbl_con_status.Dispatcher.Invoke(new Action(() =>
                { lbl_con_status.Content = "Loading FEA bits in Flash ..                  "; }));
                //            labelSystem_status.Text = "Loading FEA bits in Flash ";
                //           labelSystem_status.Update();

                new_lines[0] = file.ReadLine();

                //progress 89
                // convert line string to binary
                k = 0;
                remaining = 99 - prt;
                for (j = 0; j < 2; j++)
                {
                    UpdateProgress((int)(prt += ((remaining / 3) - 1)));
                    Thread.Sleep(40);
                    temp = new_lines[0].Substring(k, 8);
                    data_bytes[j] = Convert.ToByte(temp, 2);
                    k += 8;
                }

                //Progress 97
                usb_write(PROFILE, data_bytes, 2);

                // wait for response to let us know controler is ready
                while (cpld_data_recieved_flag == 0) { };
                cpld_data_recieved_flag = 0;
                //  data_back = read_usb_prog(6);

                if (cpld_data_recieved == 1)
                {
                    // error
                    lbl_con_status.Dispatcher.Invoke(new Action(() =>
                    { lbl_con_status.Content = "Problem updating CPLD flash.  Try again. Error 5"; }));
                    MessageBox.Show("Problem updating CPLD flash.  Try again. Error 5");
                    //start_update_gui(sender, e);
                    // _timer1.Start();
                    return;
                }
                file.Close();
                lbl_con_status.Dispatcher.Invoke(new Action(() =>
                { lbl_con_status.Content = "CPLD Flash Update Complete.        "; }));
                //labelSystem_status.Text = "CPLD Flash Update Complete.        ";
                // labelSystem_status.Update();
                // start_update_gui(sender, e);
                //button1.IsEnabled = true;
                //button1.Content = "Update";
                usb_disconnect();

            }
            catch (Exception)
            {
                dbg1++;
            }
            finally
            {
                _txScheduler.CooldownMs = cooldownBackup;
                SetProfileLock(string.Empty);
            }
        }

        private void UpdateProgress_CPLD(int percent)
        {
            if (null != _worker)
            {
                _worker.ReportProgress(percent);
            }
        }

        void _worker_RunWorkerCompleted_CPLD(object sender, RunWorkerCompletedEventArgs e)
        {
            _timer1.Enabled = true;
            pbar_cpld.Visibility = Visibility.Hidden;
            lbl1pbar_cpld.Visibility = Visibility.Hidden;
            // reenable tabs and controls
            MCM6000_tabs.IsEnabled = true;
            // this.Close();
        }

        void _worker_ProgressChanged_CPLD(object sender, ProgressChangedEventArgs e)
        {
            pbar_cpld.Value = e.ProgressPercentage;
            lbl1pbar_cpld.Content = e.ProgressPercentage.ToString() + "%";
        }



    }
}