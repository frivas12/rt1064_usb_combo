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
namespace MCM6000_UI
{
    public partial class MainWindow
    {
        static double[,] mark = new double[3, 4] { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };

        bool goto_block = false;
        static byte mirror_position = 0;

        private void btnToggle_Block_Goto_Click(object sender, RoutedEventArgs e)
        {
            Visibility _VisibleState = Visibility.Hidden;

            // toglle disable all the goto buttons
            // block (hide) goto commands
            if (goto_block == false)
            {
                btnToggle_Block_Meso.Content = "Unblock Buttons";
                _VisibleState = Visibility.Hidden;
                goto_block = true;
            }
            else
            {
                btnToggle_Block_Meso.Content = "Block Buttons";
                _VisibleState = Visibility.Visible;
                goto_block = false;
            }


            btnSet_Goto_1.Visibility = _VisibleState;
            btnSet_Goto_2.Visibility = _VisibleState;
            btnSet_Goto_3.Visibility = _VisibleState;
            btnGoto_1.Visibility = _VisibleState;
            btnGoto_2.Visibility = _VisibleState;
            btnGoto_3.Visibility = _VisibleState;
            btnZero_1.Visibility = _VisibleState;
            btnZero_2.Visibility = _VisibleState;
            btnZero_3.Visibility = _VisibleState;
            btnZero_4.Visibility = _VisibleState;
            btnMeso_CCW_softlim_1.Visibility = _VisibleState;
            btnMeso_CCW_softlim_2.Visibility = _VisibleState;
            btnMeso_CCW_softlim_3.Visibility = _VisibleState;
            btnMeso_CCW_softlim_4.Visibility = _VisibleState;
            btnMeso_CW_softlim_1.Visibility = _VisibleState;
            btnMeso_CW_softlim_2.Visibility = _VisibleState;
            btnMeso_CW_softlim_3.Visibility = _VisibleState;
            btnMeso_CW_softlim_4.Visibility = _VisibleState;
            btnMeso_clear_softlim_1.Visibility = _VisibleState;
            btnMeso_clear_softlim_2.Visibility = _VisibleState;
            btnMeso_clear_softlim_3.Visibility = _VisibleState;
            btnMeso_clear_softlim_4.Visibility = _VisibleState;
            btnMeso_CCW_F_1.Visibility = _VisibleState;
            btnMeso_CCW_F_2.Visibility = _VisibleState;
            btnMeso_CCW_F_3.Visibility = _VisibleState;
            btnMeso_CCW_F_4.Visibility = _VisibleState;
            btnMeso_CCW_S_1.Visibility = _VisibleState;
            btnMeso_CCW_S_2.Visibility = _VisibleState;
            btnMeso_CCW_S_3.Visibility = _VisibleState;
            btnMeso_CCW_S_4.Visibility = _VisibleState;
            btnMeso_CW_S_1.Visibility = _VisibleState;
            btnMeso_CW_S_2.Visibility = _VisibleState;
            btnMeso_CW_S_3.Visibility = _VisibleState;
            btnMeso_CW_S_4.Visibility = _VisibleState;
            btnMeso_CW_F_1.Visibility = _VisibleState;
            btnMeso_CW_F_2.Visibility = _VisibleState;
            btnMeso_CW_F_3.Visibility = _VisibleState;
            btnMeso_CW_F_4.Visibility = _VisibleState;

            clear_focus();
        }

        private void btnSet_Mark_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string mark_num = button_name.Substring(button_name.Length - 1, 1);
            Byte mark_number = Convert.ToByte(mark_num);
            mark_number = Convert.ToByte(mark_number - 1);

            float num_x = float.Parse(lblXPos.Text);
            float num_y = float.Parse(lblYPos.Text);
            float num_z = float.Parse(lblZPos.Text);
            float num_r = float.Parse(lblRPos.Text);
            mark[mark_number, 0] = num_x;
            mark[mark_number, 1] = num_y;
            mark[mark_number, 2] = num_z;
            mark[mark_number, 3] = num_r;

            if (mark_number == 0)
            {
                lb_mark_x1.Content = lblXPos.Text;
                lb_mark_y1.Content = lblYPos.Text;
                lb_mark_z1.Content = lblZPos.Text;
                lb_mark_r1.Content = lblRPos.Text;
            }
            else if (mark_number == 1)
            {
                lb_mark_x2.Content = lblXPos.Text;
                lb_mark_y2.Content = lblYPos.Text;
                lb_mark_z2.Content = lblZPos.Text;
                lb_mark_r2.Content = lblRPos.Text;
            }
            else
            {
                lb_mark_x3.Content = lblXPos.Text;
                lb_mark_y3.Content = lblYPos.Text;
                lb_mark_z3.Content = lblZPos.Text;
                lb_mark_r3.Content = lblRPos.Text;
            }

            clear_focus();
        }

        private void btnGoto_Mark_Click(object sender, RoutedEventArgs e)
        {
            Byte slot_id = 0;
            byte[] bytesToSend;
            double value = 0;
            byte[] data1;

            _timer1.Enabled = false;
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_MOVE_ABSOLUTE));

            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string mark_num = button_name.Substring(button_name.Length - 1, 1);
            Byte mark_number = Convert.ToByte(mark_num);
            mark_number = Convert.ToByte(mark_number - 1);

            for (Byte slot_number = 0; slot_number < 4; slot_number++)
            {
                slot_id = (Byte)(slot_number + 1 | 0x20);

                value = mark[mark_number, slot_number];

                value = (1000 * value) / slot_nm_per_count[slot_number];
                if (!double.IsNaN(value))
                {
                    data1 = BitConverter.GetBytes(Convert.ToInt32(value));

                    bytesToSend = new byte[12] { command[0], command[1], 0x06, 0x00, Convert.ToByte(slot_id | 0x80), HOST_ID,
                    slot_number, 0x00,
                    data1[0], data1[1], data1[2], data1[3]
                    };

                    usb_write("meso-tab", bytesToSend, 12);
                    Thread.Sleep(50);
                }
            }
            _timer1.Enabled = true;
            clear_focus();
        }

        private void btnMeso_Zero_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_SET_ENCCOUNTER));

            byte[] bytesToSend = new byte[12] { command[0], command[1], 0x06, 0x00, Convert.ToByte(slot_id | 0x80), HOST_ID,
                slot_number, 0x00,
                0x00, 0x00, 0x00, 0x00 };

            usb_write("meso-tab", bytesToSend, 12);
            clear_focus();
        }


        private void btnToggle_mirror_Click(object sender, RoutedEventArgs e)
        {
            if (mirror_position == 1)
                mirror_position = 2;
            else if (mirror_position == 2)
                mirror_position = 1;
            else
                mirror_position = 1;

            Byte slot_id = (Byte)((7 | 0x20) | 0x80);   // servo card on slot 7
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_SET_BUTTONPARAMS));

            byte[] bytesToSend = new byte[22] { command[0], command[1], 16, 0x00, slot_id, HOST_ID,
                5, 0x00,
                0,0,            // Mode (not used)
                mirror_position, 0, 0, 0,     // Position 1 (for Mezoscope 1 = position 1, 2 = position 2)
                0,0,0,0,        // Position 2  (not used)
                0,0,            // TimeOut (not used)
                0,0            // (not used)
            };

            usb_write("meso-tab", bytesToSend, 22);

            this.Dispatcher.Invoke(new Action(() =>
            {

                switch (mirror_position)
                {
                    case 0:     // 0, doesn't know where it is
                        btnToggle_smirror.Foreground = new SolidColorBrush(Colors.Red);
                        btnToggle_smirror.Content = "Position Unknown";
                        break;

                    case 1:     // 1 = position 1
                        btnToggle_smirror.Foreground = new SolidColorBrush(Colors.White);
                        btnToggle_smirror.Content = "Mirror Closed";
                        break;

                    case 2:     // 2 = position 2
                        btnToggle_smirror.Foreground = new SolidColorBrush(Colors.White);
                        btnToggle_smirror.Content = "Mirror Open";
                        break;
                }
            }));
            clear_focus();
        }
        private void btnToggle_shutter_Click(object sender, RoutedEventArgs e)
        {
            Byte slot_number = SLOT6;
            Byte slot_id = (Byte)((slot_number + 1 | 0x20));   // servo card on slot 6
            byte state = 0;

            if (btnToggle_shutter.Content.ToString() == "Shutter Closed")
            {
                // open the shutter
                btnToggle_shutter.Dispatcher.Invoke(new Action(() =>
                { btnToggle_shutter.Content = "Shutter Opened"; }));
                state = SHUTTER_OPENED;
            }
            else
            {
                // close the shutter
                btnToggle_shutter.Dispatcher.Invoke(new Action(() =>
                { btnToggle_shutter.Content = "Shutter Closed"; }));
                state = SHUTTER_CLOSED;
            }

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_SET_SOL_STATE));

            byte[] bytesToSend = new byte[6] { command[0], command[1], slot_number, state, slot_id, HOST_ID };
            usb_write("meso-tab", bytesToSend, 6);

            clear_focus();
        }

        public void meso_get_joystick_data_selected_slot()
        {
            byte[] js_slot_select = null;

            js_slot_select = new byte[4]; // upto for ports

            js_slot_select[0] = ext_data[0];     //port 1 or port 2 on a hub
            js_slot_select[1] = ext_data[1];     // port 3 on a hub
            js_slot_select[2] = ext_data[2];     // port 4 on a hub
            js_slot_select[3] = ext_data[3];     // port 5 on a hub

            // make the label background green for the active slot
            this.Dispatcher.Invoke(new Action(() =>
            {
                // slot 1
                if ((js_slot_select[0] == 1) || (js_slot_select[1] == 1)
                 || (js_slot_select[2] == 1) || (js_slot_select[3] == 1))
                    meso_x_lb.Background = Brushes.DarkGreen;
                else
                    meso_x_lb.Background = new SolidColorBrush(Color.FromRgb(58, 59, 60));

                // slot 2
                if ((js_slot_select[0] == 2) || (js_slot_select[1] == 2)
                 || (js_slot_select[2] == 2) || (js_slot_select[3] == 2))
                    meso_y_lb.Background = Brushes.DarkGreen;
                else
                    meso_y_lb.Background = new SolidColorBrush(Color.FromRgb(58, 59, 60));

                // slot 3
                if ((js_slot_select[0] == 4) || (js_slot_select[1] == 4)
                 || (js_slot_select[2] == 4) || (js_slot_select[3] == 4))
                    meso_z_lb.Background = Brushes.DarkGreen;
                else
                    meso_z_lb.Background = new SolidColorBrush(Color.FromRgb(58, 59, 60));

                // slot 4
                if ((js_slot_select[0] == 8) || (js_slot_select[1] == 8)
                 || (js_slot_select[2] == 8) || (js_slot_select[3] == 8))
                    meso_r_lb.Background = Brushes.DarkGreen;
                else
                    meso_r_lb.Background = new SolidColorBrush(Color.FromRgb(58, 59, 60));

            }));

        }

    }
}