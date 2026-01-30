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

        //    static SerialPort _serialPort;
        //static string address;

        /*SAMS70 MICROCONTROLLER MEMORY VARIABLES*/

        static uint address_offset_for_mcm = 0;
        static uint HEX_FILE_adrs_for_mcm = 0;
        static bool MCM6000 = false;
        const uint BASE_ADDRESS_FOR_MCM6000 = 0x400000;
        const int FLASH_SIZE_IN_BYTES = 1966080;// Flash Size = 2MB of flash - 128KB for Bootloader
        public const int MAX_PAGES = 3840;
        const int Address_Bytes = 4;

        bool firmware_error = false;

        /// The _firmware_output
        byte[][] firmware_output; //up to 1 MB flash space in 64byte pages, leading 4 bytes are 32bit address

        /// The _worker
        BackgroundWorker _worker;

        private void btn_select_firmware_file_Click(object sender, RoutedEventArgs e)
        {
            if (!_programmingPath.ProvidesFirmware)
            { return; }

            loading_firmware = false;
            try
            {
                pbar.Visibility = Visibility.Visible;
                lbl1pbar.Visibility = Visibility.Visible;
                lbl_con_status1.Visibility = Visibility.Visible;

                _worker = new();
                _worker.WorkerReportsProgress = true;
                _worker.RunWorkerCompleted += _worker_RunWorkerCompleted;
                _worker.ProgressChanged += _worker_ProgressChanged;
                _worker.DoWork += (sender, e) =>
                {
                    loading_firmware = true;
                    lbl_con_status.Dispatcher.Invoke(new Action(() =>
                    {
                        lbl_con_status.Content = "Loading firmware";
                        lbl_con_status.Foreground = Brushes.Red;
                        MCM6000_tabs.IsEnabled = false;
                    }));
                    using (var stream = _programmingPath.OpenFirmware())
                    {
                        FlashWriteandValidate(stream);
                    }
                    UpdateProgress(100); // -1 is the index used to update the status the label
                    Thread.Sleep(1500);
                };
                _worker.RunWorkerAsync();
            }
            catch (Exception)
            { }
        }

        private void btnEnter_Exit_bootloader_Click(object sender, RoutedEventArgs e)
        {
            if ("Enter Bootloader" == (string)btEnter_Exit_BootLoader.Content)
            {
                this.Dispatcher.Invoke(new Action(() =>
                {
                    btEnter_Exit_BootLoader.Content = "Enter Application";
                    btn_select_firmware_file.Visibility = Visibility.Visible;
                    labelFirmware_rev_val.Visibility = Visibility.Visible;
                }));

                byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_GET_UPDATE_FIRMWARE));
                byte[] bytesToSend = [command[0], command[1], 1, 0x00, MOTHERBOARD_ID, HOST_ID];
                usb_write("firmware-updater", bytesToSend, 6);
                in_bootloader = true;
                usb_disconnect();
                Thread.Sleep(3000);
            }
            else
            {
                this.Dispatcher.Invoke(new Action(() =>
                {
                    this.Dispatcher.Invoke(new Action(() =>
                    {
                        btEnter_Exit_BootLoader.Content = "Enter Bootloader";
                        btn_select_firmware_file.Visibility = Visibility.Hidden;
                        lbl1pbar.Visibility = Visibility.Hidden;
                        lbl_con_status1.Visibility = Visibility.Hidden;
                        pbar.Visibility = Visibility.Hidden;
                        labelFirmware_rev_val.Visibility = Visibility.Hidden;
                    }));

                    byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_RESET_FIRMWARE_LOADCOUNT));
                    byte[] bytesToSend = new byte[6] { command[0], command[1], 0, 0x00, MOTHERBOARD_ID, HOST_ID };
                    usb_write("firmware-updater", bytesToSend, 6);
                    in_bootloader = false;
                    usb_disconnect();
                    System.Threading.Thread.Sleep(3000);
                }));
            }
        }

        uint intel_hex_conversion(Stream stream)
        {
            //initialize array to 0xff
            for (int c = 0; c < MAX_PAGES; c++)
            {
                firmware_output[c] = new byte[S70_PAGE_SIZE + 4];
                for (int d = 0; d < (S70_PAGE_SIZE + 4); d++)
                {
                    firmware_output[c][d] = 0xff;
                }
            }
            /*
            */
            byte[] temp_buffer = new byte[FLASH_SIZE_IN_BYTES];  //TOTAL SIZE OF FLASH = (2MB - 128KB)
            for (int c = 0; c < FLASH_SIZE_IN_BYTES; c++)
            {
                temp_buffer[c] = 0xff;
            }

            System.IO.StreamReader file_in = new System.IO.StreamReader(stream);

            string temp_line;
            uint byte_count, address, record_type, checksum, address_offset = 0;
            uint min_address = 0xffffffff, max_address = 0x00000000;
            uint dg = 0;
            do
            {
                temp_line = file_in.ReadLine();  //read next line from intel hex file
                dg++;

                byte_count = Convert.ToUInt16(temp_line.Substring(1, 2), 16);  //first byte after :
                address = Convert.ToUInt16(temp_line.Substring(3, 4), 16);  //second and third bytes
                record_type = Convert.ToUInt16(temp_line.Substring(7, 2), 16);  //fourth byte
                checksum = Convert.ToUInt16(temp_line.Substring(temp_line.Length - 2, 2), 16); //last byte

                address = address & 0xffff;  //set upper bytes of 32 bit int to zeros

                if (record_type == 0x02)  //it's an extended segment address
                {
                    address_offset = Convert.ToUInt16(temp_line.Substring(9, 4), 16);  //offset data from ext seg address
                    address_offset = address_offset << 4;
                }
                if (record_type == 0x05)
                {
                    max_address = max_address + 16;
                }
                if (record_type == 0x00)  //data that needs to be sent
                {
                    if (MCM6000)
                    {
                        for (int c = 0; c < byte_count; c++)  //convert all data bytes in the line and load the firmware buffer
                        {
                            temp_buffer[address + address_offset_for_mcm + c] = Convert.ToByte(temp_line.Substring(9 + c * 2, 2), 16);  //convert each pair of chars to byte
                        }
                        if ((address + address_offset_for_mcm) < min_address) { min_address = address + address_offset_for_mcm; }
                        if ((address + address_offset_for_mcm) > max_address) { max_address = address + address_offset_for_mcm; }
                    }

                }
                /* 
                */
                if ((record_type == 0x04))
                {
                    MCM6000 = true;
                    HEX_FILE_adrs_for_mcm = Convert.ToUInt32(temp_line.Substring(9, (int)byte_count * 2), 16);
                    HEX_FILE_adrs_for_mcm = HEX_FILE_adrs_for_mcm << 16;
                    address_offset_for_mcm = HEX_FILE_adrs_for_mcm - BASE_ADDRESS_FOR_MCM6000;
                }

            } while (record_type != 0x01);  //run til eof

            uint total_byte_count = max_address - min_address;
            uint total_pages = (total_byte_count - 1) / S70_PAGE_SIZE + 1;  //-1 and +1 are to round up the next whole page

            address = min_address;

            for (int c = 0; c < total_pages; c++)
            {

                Array.Copy(BitConverter.GetBytes(address), firmware_output[c], 4);
                Array.Copy(temp_buffer, address, firmware_output[c], 4, S70_PAGE_SIZE);
                int addressFW = firmware_output[c][0] | firmware_output[c][1] << 8 | firmware_output[c][2] << 16 | firmware_output[c][3] << 24;
                address += S70_PAGE_SIZE;
            }

            file_in.Close();
            return total_pages;

        }
        /*
         Write page by page data to flash
         and verify it.
         This needs to be done for all firmware pages.
         */
        private void FlashWriteandValidate(Stream stream)
        {
            firmware_output = new byte[MAX_PAGES][]; //up to 1 MB flash space in 64byte pages, leading 4 bytes are 32bit address

            uint page_cnt = intel_hex_conversion(stream);
            byte[] out_buffer = new byte[S70_PAGE_SIZE + 10]; // was 74 for samd21
            byte[] test_buffer = new byte[70];
            /*
            0x00A8 is MGMSG_BL_SET_FLASHPAGE command.
            */
            out_buffer[0] = 0xA8;  //command lower byte
            out_buffer[1] = 0x00;  //command upper byte
            //Total number of bytes = 512 bytes of data + 4 bytes for address = 516 bytes
            out_buffer[2] = ((S70_PAGE_SIZE + Address_Bytes) & 0xff);  //number of data bytes lower // was 0x44 BEFORE FOR SAMD21
            out_buffer[3] = ((S70_PAGE_SIZE + Address_Bytes) >> 8);  //number of data bytes upper 
            out_buffer[4] = Convert.ToByte(MOTHERBOARD_ID | 0x80);  //destination
            out_buffer[5] = HOST_ID;  //source

            /*
            0x00A7 is MGMSG_RESET_FIRMWARE_LOADCOUNT command used to take a jump from Bootloader mode to Application mode.
            Only when Flash writing and validation succeeeds, this command is issued in the end.
            */
            byte[] reset_load_command_buffer = new byte[6];
            reset_load_command_buffer[0] = 0xA7;  //command lower byte
            reset_load_command_buffer[1] = 0x00;  //command upper byte
            reset_load_command_buffer[2] = 0x00;  //number of data bytes lower  00
            reset_load_command_buffer[3] = 0x00;  //number of data bytes upper 
            reset_load_command_buffer[4] = MOTHERBOARD_ID;  //destination
            reset_load_command_buffer[5] = HOST_ID;  //source

            bool NEXT_ACT = false;
            while (!NEXT_ACT)
            {
                //address
                int start_address = firmware_output[0][0] | firmware_output[0][1] << 8 | firmware_output[0][2] << 16 | firmware_output[0][3] << 24;
                if (start_address == 0x20000)
                    NEXT_ACT = true;
                else
                    break;
            }
            double totalProcessCount = page_cnt; // page_cnt * 2 + 16535 / 10.0;
            double progressCount = 0;
            if (NEXT_ACT)
            {
                if (page_cnt < MAX_PAGES)
                {
                    int dotj = 0;
                    for (int c = 0; c < page_cnt; c++)
                    {
                        //if(c == page_cnt - 1)
                        //{
                        //    dd++;
                        // }
                        int addressFW = firmware_output[c][0] | firmware_output[c][1] << 8 | firmware_output[c][2] << 16 | firmware_output[c][3] << 24;

                        Array.Copy(firmware_output[c], 0, out_buffer, 6, S70_PAGE_SIZE + 4); // cpying firmware output into out_buffer // was 68 for samd21
                        usb_write("firmware-updater", out_buffer, S70_PAGE_SIZE + 10); // out_buffer = THORLABS APT + page_adrs + 512 bytes data. Was 74 for samd21
                        Thread.Sleep(20);
                        firmware_transfer_done.WaitOne();
                        byte[] input = ext_data;// usb_wait_read(S70_PAGE_SIZE + 6); //Receive 512 bytes from firmware. This is to validate ROM.
                        progressCount++;
                        dotj++;
                        if (dotj % 3 == 1)
                            lbl_con_status1.Dispatcher.Invoke(new Action(() =>
                            { lbl_con_status1.Content = "Flash Writing and Validation. "; }));
                        if (dotj % 3 == 2)
                            lbl_con_status1.Dispatcher.Invoke(new Action(() =>
                            { lbl_con_status1.Content = "Flash Writing and Validation.. "; }));
                        if (dotj % 3 == 0)
                            lbl_con_status1.Dispatcher.Invoke(new Action(() =>
                            { lbl_con_status1.Content = "Flash Writing and Validation... "; }));
                        Thread.Sleep(1);


                        if (0 == progressCount % 25)
                        {
                            UpdateProgress((int)((progressCount * 100) / totalProcessCount));
                            System.Threading.Thread.Sleep(5);
                        }

                        for (int ck = 0; ck < S70_PAGE_SIZE; ck++)
                        {
                            if (input[ck + 6] != out_buffer[ck + 10])
                            {
                                MessageBox.Show("Error at page number" + c.ToString());
                                firmware_error = true;

                            }
                            if (firmware_error)
                                break;
                        }
                        if (firmware_error)
                            break;
                    }

                    //Check till here now. Third Step
                    if (firmware_error)
                    {
                        MessageBox.Show("Firmware Upgrade failed");
                        //serialPort2.Close();
                    }
                    else
                    {
                        //NO ERROR SO JUMP TO APPLICATION
                        usb_write("firmware-updater", reset_load_command_buffer, 6);
                        UpdateProgress(100);
                        System.Threading.Thread.Sleep(5);
                    }

                }
                else
                {
                    MessageBox.Show("Firmware Overwriting to flash addresses. Firmware Upgrade failed");
                }
            }
            else
            {
                MessageBox.Show("Firmware Starts at incorrect address. Firmware Upgrade failed");
            }
        }

        /// <summary>
        /// Updates the progress.
        /// </summary>
        /// <param name="percent">The percent.</param>
        /// <param name="deviceIndex">Index of the device.</param>
        private void UpdateProgress(int percent)
        {
            if (null != _worker)
            {
                _worker.ReportProgress(percent);
            }
        }

        /// <summary>
        /// Handles the ProgressChanged event of the _worker control.
        /// the index (userState) is used to update the right progress bar
        /// </summary>
        /// <param name="sender">The source of the event.</param>
        /// <param name="e">The <see cref="ProgressChangedEventArgs"/> instance containing the event data.</param>
        void _worker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            lbl1pbar.Dispatcher.Invoke(new Action(() =>
            {
                lbl1pbar.Content = e.ProgressPercentage.ToString() + "%";
            }));

            pbar.Dispatcher.Invoke(new Action(() =>
            {
                pbar.Value = e.ProgressPercentage;
            }));

        }

        /// <summary>
        /// Handles the RunWorkerCompleted event of the _worker control.
        /// </summary>
        /// <param name="sender">The source of the event.</param>
        /// <param name="e">The <see cref="RunWorkerCompletedEventArgs"/> instance containing the event data.</param>
        void _worker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            // restart the application and close this window
            System.Diagnostics.Process.Start(Application.ResourceAssembly.Location);
            Application.Current.Shutdown();
        }
    }
}