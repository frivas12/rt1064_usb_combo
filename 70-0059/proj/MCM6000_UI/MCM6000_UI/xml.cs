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
using System.Diagnostics;
using System.Xml;
using SystemXMLParser;

namespace MCM6000_UI
{

    public enum xmlTypes
    {
        XML_SYSTEM = 0,
        XML_STAGE,
        XML_JOYSTICK,
        XML_SHUTTER,
        XML_SERVO,  // these are for the 41-0098 servo card which was used for meso flipper mirro and the shutter 
        XML_SLIDER_IO,
        XML_SHUTTER_4,  // 6
        XML_PIEZO,  // 7
        XML_ONEWIRE,
        XML_FLIPPER_SHUTTER,
    }

    public partial class MainWindow
    {
        const byte XML_SEND_DELAY = 100;
        
        /// The _worker
        BackgroundWorker xml_worker;
        int xml_error = 0;

        bool USB_reading = false;

        private void read_XML(xmlTypes type, byte number)
        {
            //Pop a dialog to get the XML file location
            Microsoft.Win32.OpenFileDialog dlg = new Microsoft.Win32.OpenFileDialog();
            dlg.Title = "Select system level XML file";
            dlg.DefaultExt = ".xml";
            dlg.Filter = "XML Files|*.xml";
            Nullable<bool> result = dlg.ShowDialog();
            string top_level_xml = dlg.FileName;

            if (result == false)
            {
                xml_error = 1;
                return;
            }

            SystemXMLParser.ConfigurationParams args = new()
            {
                DEFAULT_TIMEOUT = 2000,
                usb_send = (b) => { usb_write("system-xml-parser", b, b.Length); Thread.Sleep(15); },
                efs_driver = _efs_driver,
            };

            switch (type)
            {
                case xmlTypes.XML_SYSTEM: // configure an entire system
                    {
                        // Config card type (save type to card eeprom)

                        bool slot_type_flag = false;
                        bool allowed_flag = false;
                        bool lut_flag = false;
                        bool joystick_config_flag = false;
                        bool custom_config_flag = false;

                        SystemXMLParser.ConfigurationParams lut_args = new()
                        {
                            DEFAULT_TIMEOUT = args.DEFAULT_TIMEOUT,
                            usb_send = args.usb_send,
                            usb_response = () => { m_lut_response_ready.WaitOne(); return m_lut_response_buffer; },
                            usb_timeout = (t) => { return m_lut_response_ready.WaitOne(t) ? m_lut_response_buffer : null; },
                            efs_driver = _efs_driver
                        };

                        this.Dispatcher.Invoke((() =>
                        {
                            slot_type_flag = e_cbx_xml_load_slots.IsChecked == true;
                            allowed_flag = e_cbx_xml_load_allowed_devices.IsChecked == true;
                            lut_flag = e_cbx_xml_load_lut.IsChecked == true;
                            joystick_config_flag = e_cbx_xml_load_joystick.IsChecked == true;
                            custom_config_flag = slot_type_flag;
                        }));

                        SystemXMLParser.SystemXML? xml = null;
                        try
                        {
                            xml = new(top_level_xml);
                        }
                        catch (Exception ex)
                        {
                            Dispatcher.Invoke(new Action(() => { lb_load_xml_status.Content = ex.Message; }));
                            slot_type_flag = false;
                            allowed_flag = false;
                            lut_flag = false;
                            joystick_config_flag = false;
                            xml_error = 1;
                            MessageBox.Show(ex.Message);
                        }

                        if (lut_flag && !_programmingPath.ProvidesLUT)
                        {
                            xml_error = 1;
                            if (_programmingPath.Path.Length == 0)
                            {
                                MessageBox.Show("No Programming File has been selected.");
                            }
                            else
                            {
                                MessageBox.Show("The Programming File must provide the LUT to perform configuration.");
                            }
                        }
                        else
                        {

                            if (slot_type_flag)
                            {
                                Dispatcher.Invoke(new Action(() => { lb_load_xml_status.Content = "Loading slot card types"; }));

                                try
                                {
                                    xml.XmlSettings.Configure(args);
                                }
                                catch (Exception ex)
                                {
                                    xml_error = 1;
                                    MessageBox.Show(ex.Message);
                                }

                                Restart_processor(false);
                                _timer1.Enabled = true;
                                USB_reading = false;
                                while (!USB_reading)
                                {
                                    Thread.Sleep(100);
                                }
                                _timer1.Enabled = false;
                                Thread.Sleep(1500);
                            }

                            if (custom_config_flag)
                            {
                                Dispatcher.Invoke(new Action(() => { lb_load_xml_status.Content = "Loading custom configurations"; }));

                                // Load any custom configurations after the restart.
                                try
                                {
                                    xml.XmlSettings.ConfigureCustomParams(args);
                                }
                                catch (Exception ex)
                                {
                                    xml_error = 1;
                                    MessageBox.Show(ex.Message);
                                }

                                Thread.Sleep(500);
                            }

                            // Set allowed devices
                            if (allowed_flag)
                            {
                                Dispatcher.Invoke(new Action(() => { lb_load_xml_status.Content = "Loading allowed devices"; }));

                                try
                                {
                                    xml.XmlAllowedDevices.Configure(args);
                                }
                                catch (Exception ex)
                                {
                                    xml_error = 1;
                                    MessageBox.Show(ex.Message);
                                }

                                Thread.Sleep(1000);
                            }

                            // Set LUTs
                            if (lut_flag)
                            {
                                Dispatcher.Invoke(new Action(() => { lb_load_xml_status.Content = "Loading device lut"; }));

                                void ProgramEFSFile(Stream source, string fileName, byte id, EFSAttributes attributes = null)
                                {
                                    byte[] data;
                                    using (var mstream = new MemoryStream())
                                    {
                                        source.CopyTo(mstream);
                                        data = mstream.GetBuffer().Take((int)mstream.Length).ToArray();
                                    }

                                    attributes ??= new(true, true, true, true, false, false);

                                    var driver = lut_args.efs_driver;
                                    driver.DeleteFileAsync(id).Wait();
                                    driver.CreateFile(id, attributes, (uint)data.Length).Wait();
                                    EFSFile? file = lut_args.efs_driver.GetFile(id)
                                        ?? throw new Exception("Could not create the file");
                                    file.Write(0, data);
                                    var task = file.ReadAsync(0, data.Length);
                                    task.Wait();
                                    byte[] read_back = task.Result;
                                    if (! read_back.SequenceEqual(data))
                                    {
                                        throw new Exception(fileName + " mismatched");
                                    }
                                }

                                LutSetLock(false);
                                Thread.Sleep(10);

                                try
                                {
                                    foreach (var id in _programmingPath.LUTKeys)
                                    {
                                        string name = id switch
                                        {
                                            96 => "device lut",
                                            97 => "config lut",
                                            98 => "lut version metadata",
                                            _ => $"efs file 0x{id:X02}"
                                        };

                                        using var stream = _programmingPath.OpenLUT(id);

                                        Dispatcher.Invoke(new Action(() => { lb_load_xml_status.Content = "Loading " + name; }));
                                        ProgramEFSFile(stream, name, id);
                                        Thread.Sleep(1000);
                                    }
                                }
                                catch (Exception ex)
                                {
                                    xml_error = 1;
                                    MessageBox.Show(ex.Message);
                                }
                                finally
                                {
                                    LutSetLock(true);
                                    Thread.Sleep(10);
                                }
                            }

                            // load Joystick config mappings
                            if (joystick_config_flag == true)
                            {
                                Dispatcher.Invoke(new Action(() => { lb_load_xml_status.Content = "Loading joystick configurations"; }));



                                try
                                {
                                    xml.XmlJoysticks.Configure(args, true);
                                }
                                catch (Exception ex)
                                {
                                    xml_error = 1;
                                    MessageBox.Show(ex.Message);
                                }
                                Thread.Sleep(1000);
                            }
                        }
                    }
                    break;

                case xmlTypes.XML_STAGE:
                    {
                        SystemXMLParser.StepperXML? xml = null;
                        bool save_to_eeprom = false;
                        Dispatcher.Invoke(() => {
                            save_to_eeprom = e_cbx_stepper_xml_persistent.IsChecked == true;
                        });
                        try
                        {
                            xml = new(top_level_xml, number - 0x20);

                            xml.Configure(args, save_to_eeprom);
                        }
                        catch (Exception ex)
                        {
                            xml_error = 1;
                            MessageBox.Show(ex.Message);
                        }
                    }
                    break;

                case xmlTypes.XML_JOYSTICK:
                    {
                        SystemXMLParser.SystemXML.Joysticks.Port? xml = null;
                        bool save_to_eeprom = false;
                        Dispatcher.Invoke(() => {
                            save_to_eeprom = e_chk_system_save_js.IsChecked == true;
                        });

                        try
                        {
                            xml = new(top_level_xml, number);

                            xml.Configure(args, save_to_eeprom);
                        }
                        catch (Exception ex)
                        {
                            xml_error = 1;
                            MessageBox.Show(ex.Message);
                        }
                    }
                    break;

                //case xmlTypes.XML_SHUTTER:
                //    slot_num = number;
                //    parameters = opened_xml.DocumentElement.SelectSingleNode("/ShutterParameters");
                //    //loop through each type of parameter, each one is a separate command
                //    foreach (XmlNode parameter_type in parameters.ChildNodes)
                //    {
                //        Send_XML_APT_Command(parameter_type, slot_num, 255);
                //    }
                //    break;

                //case xmlTypes.XML_SERVO:
                //    slot_num = number;
                //    parameters = opened_xml.DocumentElement.SelectSingleNode("/ServoParameters");
                //    //loop through each type of parameter, each one is a separate command
                //    foreach (XmlNode parameter_type in parameters.ChildNodes)
                //    {
                //        Send_XML_APT_Command(parameter_type, slot_num, 255);
                //    }
                //    break;

                //case xmlTypes.XML_SLIDER_IO:
                //    slot_num = number;
                //    parameters = opened_xml.DocumentElement.SelectSingleNode("/Slider_IO_Parameters");
                //    //loop through each type of parameter, each one is a separate command
                //    foreach (XmlNode parameter_type in parameters.ChildNodes)
                //    {
                //        Send_XML_APT_Command(parameter_type, slot_num, 255);
                //    }
                //    break;

                //case xmltypes.xml_shutter_4:
                //    slot_num = number;
                //    parameters = opened_xml.documentelement.selectsinglenode("/shutterparameters");
                //    //loop through each type of parameter, each one is a separate command
                //    foreach (xmlnode parameter_type in parameters.childnodes)
                //    {
                //        send_xml_apt_command(parameter_type, slot_num, 255);
                //    }
                //    break;

                //case xmlTypes.XML_PIEZO:
                //    slot_num = number;
                //    parameters = opened_xml.DocumentElement.SelectSingleNode("/PiezoParameters");
                //    //loop through each type of parameter, each one is a separate command
                //    foreach (XmlNode parameter_type in parameters.ChildNodes)
                //    {
                //        Send_XML_APT_Command(parameter_type, slot_num, 255);
                //    }
                //    break;

                case xmlTypes.XML_FLIPPER_SHUTTER:
                    {
                        FlipperShutterXML xml = null;
                        bool save_to_eeprom = false;
                        Dispatcher.Invoke(() => {
                            save_to_eeprom = e_cbx_4_shutter_xml_persistent.IsChecked == true;
                        });
                        try
                        {
                            xml = new(top_level_xml, number - 0x20);

                            xml.Configure(args, save_to_eeprom);
                        }
                        catch (Exception ex)
                        {
                            xml_error = 1;
                            MessageBox.Show(ex.Message);
                        }
                    }
                    break;

                default:
                    break;
            }
        }

        void _worker_DoWork_xml(object sender, DoWorkEventArgs e)
        {
            _timer1.Enabled = false;
            xml_error = 0;
            read_XML(xmlTypes.XML_SYSTEM, 255);

            if (xml_error == 1)
            {
                Dispatcher.Invoke(new Action(() => { lb_load_xml_status.Content = "    Loading Error    "; }));
                System.Threading.Thread.Sleep(1000);
                Dispatcher.Invoke(new Action(() => { lb_load_xml_status.Content = "*** Loading Error ***"; }));
                System.Threading.Thread.Sleep(1000);
                Dispatcher.Invoke(new Action(() => { lb_load_xml_status.Content = "    Loading Error    "; }));
                System.Threading.Thread.Sleep(1000);
                Dispatcher.Invoke(new Action(() => { lb_load_xml_status.Content = "*** Loading Error ***"; }));
                System.Threading.Thread.Sleep(1000);
            }
            else
            {
                Dispatcher.Invoke(new Action(() => { lb_load_xml_status.Content = "Loading complete"; }));
                System.Threading.Thread.Sleep(1500);
                Restart_processor(false);
            }
            Dispatcher.Invoke(new Action(() => { lb_load_xml_status.Content = ""; }));
        }

        void _worker_RunWorkerCompleted_xml(object sender, RunWorkerCompletedEventArgs e)
        {
            _timer1.Enabled = true;
        }

        private void Load_XML_Click(object sender, RoutedEventArgs e)
        {
            xml_worker = new BackgroundWorker();
            xml_worker.DoWork += new DoWorkEventHandler(_worker_DoWork_xml);
            xml_worker.RunWorkerCompleted += new RunWorkerCompletedEventHandler(_worker_RunWorkerCompleted_xml);
            xml_worker.RunWorkerAsync();
        }

        private void bt_stepper_load_xml_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                read_XML(xmlTypes.XML_STAGE, (byte)((Convert.ToByte(cb_stage_select.SelectedIndex) + 0x21)));
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
            Thread.Sleep(500);
            // Because  of device-detection, no more restarts are required.
        }
        private void bt_slider_io_load_xml_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                read_XML(xmlTypes.XML_SLIDER_IO, (byte)((Convert.ToByte(cb_io_select.SelectedIndex) + 0x21)));
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
            Thread.Sleep(500);
            Restart_processor(true);
        }

        private void bt_shutter_load_xml_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                read_XML(xmlTypes.XML_SERVO, (byte)((Convert.ToByte(cb_shutter_select.SelectedIndex) + 0x21)));
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
            Thread.Sleep(500);
            Restart_processor(true);
        }

        private void bt_servo_load_xml_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                read_XML(xmlTypes.XML_SERVO, (byte)((Convert.ToByte(cb_servo_select.SelectedIndex) + 0x21)));
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
            Thread.Sleep(500);
            Restart_processor(true);
        }

        private void bt_shutter_4_load_xml_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                int slotIndex = cb_shutter_4_select.SelectedIndex;
                xmlTypes type = card_type[slotIndex] switch
                {
                    (int)CardTypes.MCM_Flipper_Shutter => xmlTypes.XML_FLIPPER_SHUTTER,
                    (int)CardTypes.MCM_Flipper_Shutter_REVA => xmlTypes.XML_FLIPPER_SHUTTER,
                    _ => xmlTypes.XML_SHUTTER_4,
                };
                read_XML(type, (byte)((Convert.ToByte(slotIndex) + 0x21)));
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
            Thread.Sleep(500);
        }

        private void bt_joystick_load_xml_Click(object sender, RoutedEventArgs e)
        {
            byte port = (byte)(Convert.ToByte(cb_js_port.SelectedIndex));
            clear_js_in_map(port, false);
            Thread.Sleep(XML_SEND_DELAY);
            clear_js_out_map(port, false);
            Thread.Sleep(XML_SEND_DELAY);

            try
            {
                read_XML(xmlTypes.XML_JOYSTICK, port);
                Thread.Sleep(500);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private void bt_piezo_load_xml_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                read_XML(xmlTypes.XML_PIEZO, (byte)((Convert.ToByte(cb_piezo_select.SelectedIndex) + 0x21)));
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
            Thread.Sleep(5000);
            Restart_processor(true);
        }
    }
}