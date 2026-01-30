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

namespace MCM6000_UI
{
    public partial class MainWindow
    {
        public void motion_tab_init()
        {
            // Send command to get Synchonized motion settings

            Byte slot_id = 7 + 0x21;    // Synchonized motion is set to slot 8 (7 because starts on 0) 
            byte[] bytesToSend = new byte[6];

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_SYNC_MOTION_PARAM));
                bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot_id, HOST_ID };
                usb_write("motion-tab", bytesToSend, 6);
                Thread.Sleep(15);
       }

        // Response from MGMSG_MCM_REQ_SYNCHONIZED_MOTION_PARAMS
        private void synchonized_motion_get_state()
        {
            // get the syncronized motion type for the combo box 
            int sm_type = ext_data[2];

            // get the synchonized motion slot used for X axis from the combo box 
            byte x_slot = ext_data[3];

            // get the synchonized motion slot used for Y axis from the combo box 
            byte y_slot = ext_data[4];

            // get the synchonized motion slot used for Z axis from the combo box 
            byte z_slot = ext_data[5];

            Int32 synchonizedMotionDelta_t = ext_data[6] + (ext_data[7] << 8) + (ext_data[8] << 16) + (ext_data[9] << 24);

            this.Dispatcher.Invoke(new Action(() =>
                {
                    cb_synchonized_motion_type_select.SelectedIndex = sm_type;
                    cb_synchonized_motion_select_x.SelectedIndex = x_slot;
                    cb_synchonized_motion_select_y.SelectedIndex = y_slot;
                    cb_synchonized_motion_select_z.SelectedIndex = z_slot;
                    tbsynchonizedMotionDelta.Text = synchonizedMotionDelta_t.ToString();
                }));
        }

        private void bt_sm_params_refresh_Click(object sender, RoutedEventArgs e)
        {
            motion_tab_init();
        }


        private void btSavesynchonizedMotionParams_Click(object sender, RoutedEventArgs e)
        {
            Byte slot_id = (7 + 0x21) | 0x80;    // Synchonized motion is set to slot 8 (7 because starts on 0) 

            // get the syncronized motion type from the combo box 
            byte sm_type = (byte)(Convert.ToByte(cb_synchonized_motion_type_select.SelectedIndex));

            // get the synchonized motion slot used for X axis from the combo box 
            byte x_slot = (byte)(Convert.ToByte(cb_synchonized_motion_select_x.SelectedIndex));

            // get the synchonized motion slot used for Y axis from the combo box 
            byte y_slot = (byte)(Convert.ToByte(cb_synchonized_motion_select_y.SelectedIndex));

            // get the synchonized motion slot used for Z axis from the combo box 
            byte z_slot = (byte)(Convert.ToByte(cb_synchonized_motion_select_z.SelectedIndex));

            Int32 synchonizedMotionDelta_t = Int32.Parse(tbsynchonizedMotionDelta.Text);
            byte[] synchonizedMotionDelta = BitConverter.GetBytes(synchonizedMotionDelta_t);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_SYNC_MOTION_PARAM));
            byte[] bytesToSend = new byte[6 + 18] { command[0], command[1], 18, 0x00, slot_id, HOST_ID,
                0x00, 0x00, 
                sm_type,     
                x_slot,     
                y_slot,     
                z_slot,     
                synchonizedMotionDelta[0], synchonizedMotionDelta[1], synchonizedMotionDelta[2], synchonizedMotionDelta[3], 
                0,0,0,0,    // not used
                0,0,0,0    // not used
                };

            usb_write("motion-tab", bytesToSend, 6 + 18);

            Thread.Sleep(10);

        }

        private void btSmSetPoint_Click(object sender, RoutedEventArgs e)
        {
            Byte slot_id = 7 + 0x21;    // Synchonized motion is set to slot 8 (7 because starts on 0) 
            
            // get which point we are setting
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string position_set_num_s = button_name.Substring(button_name.Length - 1, 1);

            byte position_set_num = Convert.ToByte(position_set_num_s);

            byte[] bytesToSend = new byte[6];

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_SYNC_MOTION_POINT));
            bytesToSend = new byte[6] { command[0], command[1], position_set_num, 0x00, slot_id, HOST_ID };
            usb_write("motion-tab", bytesToSend, 6);
            Thread.Sleep(15);
        }

        private void btSmClearPoints_Click(object sender, RoutedEventArgs e)
        {
            Byte slot_id = 7 + 0x21;    // Synchonized motion is set to slot 8 (7 because starts on 0) 
            byte position_set_num = 0;   // zero to clear alll 3 points

            byte[] bytesToSend = new byte[6];

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_SYNC_MOTION_POINT));
            bytesToSend = new byte[6] { command[0], command[1], position_set_num, 0x00, slot_id, HOST_ID };
            usb_write("motion-tab", bytesToSend, 6);
            Thread.Sleep(15);
        }



    }

}