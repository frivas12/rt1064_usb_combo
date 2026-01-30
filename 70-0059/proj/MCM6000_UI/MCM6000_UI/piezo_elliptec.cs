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
        public void build_piezo_elliptec_control(byte _slot)
        {
            this.Dispatcher.Invoke(new Action(() =>
                       {
                           string slot_name = (_slot + 1).ToString();
                           string panel_name = "brd_slot_" + slot_name;
                           byte sub_panel_num = 0;
                           byte label_num = 0;
                           byte button_num = 0;

                           StackPanel elliptec_sel = (StackPanel)this.FindName(panel_name);

                           // Top stack panel
                           sub_panel_num = 0;
                           cp_subpanels[_slot, sub_panel_num] = new StackPanel();
                           cp_subpanels[_slot, sub_panel_num].Orientation = Orientation.Horizontal;
                           cp_subpanels[_slot, sub_panel_num].Name = "brd_slot_top_" + slot_name;
                           elliptec_sel.Children.Add(cp_subpanels[_slot, sub_panel_num]);

                           // slot label
                           Label label = new Label();
                           label.Content += "Slot " + slot_name + "     Elliptec Piezo";
                           cp_subpanels[_slot, sub_panel_num].Children.Add(label);

                           // Encoder text label
                           Label label3 = new Label();
                           label3.Content = "Encoder Position (um):";
                           cp_subpanels[_slot, sub_panel_num].Children.Add(label3);

                           // Encoder Position label
                           label_num = 1;
                           cp_labels[_slot, label_num] = new Label();
                           cp_labels[_slot, label_num].Width = 90;
                           cp_labels[_slot, label_num].Content += "";
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_labels[_slot, label_num]);

                           // 2nd row stack panel
                           sub_panel_num = 1;
                           cp_subpanels[_slot, sub_panel_num] = new StackPanel();
                           cp_subpanels[_slot, sub_panel_num].Orientation = Orientation.Horizontal;
                           elliptec_sel.Children.Add(cp_subpanels[_slot, sub_panel_num]);

                           // Home Button
                           button_num = 0;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "Home";
                           cp_button[_slot, button_num].Width = 60;
                           cp_button[_slot, button_num].Name = "elliptec_home" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_elliptec_Home_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_button[_slot, button_num]);

#if false
                           // Goto Button
                           button_num = 1;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "Goto (um)";
                           cp_button[_slot, button_num].Width = 60;
                           cp_button[_slot, button_num].Name = "elliptec_goto" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_elliptec_Goto_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_button[_slot, button_num]);

                           // Goto textbox
                           textbox_num = 0;
                           cp_textbox[_slot, textbox_num] = new TextBox();
                           cp_textbox[_slot, textbox_num].Text = "0";
                           cp_textbox[_slot, textbox_num].Width = 120;
                           cp_textbox[_slot, textbox_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_textbox[_slot, textbox_num]);
#endif
                           // Goto position 1 Button
                           button_num = 2;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "P1";
                           cp_button[_slot, button_num].Width = 40;
                           cp_button[_slot, button_num].Name = "elliptec_goto_p1_" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_elliptec_goto_stored_position_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_button[_slot, button_num]);

                           // Goto position 2 Button
                           button_num = 3;
                           cp_button[_slot, button_num] = new Button();
                           cp_button[_slot, button_num].Content = "P2";
                           cp_button[_slot, button_num].Width = 40;
                           cp_button[_slot, button_num].Name = "elliptec_goto_p2_" + slot_name;
                           cp_button[_slot, button_num].Click += new RoutedEventHandler(btn_goto_stored_position_Click);
                           cp_button[_slot, button_num].Margin = new Thickness(5, 5, 5, 5);
                           cp_subpanels[_slot, sub_panel_num].Children.Add(cp_button[_slot, button_num]);

                       }));

        }

        public void elliptec_update(byte _slot)
        {
            Byte slot_id = (Byte)((_slot + 1) | 0x20);
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_REQ_ENCCOUNTER));

            byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot_id, HOST_ID };
            usb_write("piezo_elliptec_functions", bytesToSend, 6);
        }

        public void elliptec_status_update()
        {
            double enc;

            byte _slot = ext_data[0];

            // encoder counts 
            enc = ext_data[2] | ext_data[3] << 8 | ext_data[4] << 16 | ext_data[5] << 24;
            // PR don't know why this was here 
            // enc /= 2.048;

            this.Dispatcher.Invoke(new Action(() =>
            {
                // encoder count
                cp_labels[_slot, 1].Content = enc.ToString("0,0.");
            }));

        }

        private void btn_elliptec_Home_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_MOVE_HOME));

            byte[] bytesToSend = new byte[6] { command[0], command[1], 0, 0x00, slot_id, HOST_ID };

            usb_write("piezo_elliptec_functions", bytesToSend, 6);
        }

        private void btn_elliptec_Goto_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            float num = float.Parse(cp_textbox[slot_number, 0].Text);

            string stringValue = num.ToString().Replace(',', '.');
            double value = Convert.ToDouble(stringValue);
            value = (1000 * value) / slot_nm_per_count[slot_number];

            byte[] data1 = BitConverter.GetBytes(Convert.ToInt32(value));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_MOVE_ABSOLUTE));

            byte[] bytesToSend = new byte[12] { command[0], command[1], 0x06, 0x00, Convert.ToByte(slot_id | 0x80), HOST_ID, 
               slot_number, 0x00, 
               data1[0], data1[1], data1[2], data1[3] 
            };

            usb_write("piezo_elliptec_functions", bytesToSend, 12);
        }

        private void btn_elliptec_goto_stored_position_Click(object sender, RoutedEventArgs e)
        {
            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string slot = button_name.Substring(button_name.Length - 1, 1);
            Byte slot_number = Convert.ToByte(slot);
            Byte slot_id = (Byte)(slot_number | 0x20);
            slot_number -= 1;

            // get the position number
            string pos_num_t = button_name.Substring(button_name.Length - 3, 1);
            Byte pos_num = Convert.ToByte(pos_num_t);
            pos_num -= 1;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_SET_GOTO_STORE_POSITION));

            byte[] bytesToSend = new byte[6] { command[0], command[1], slot_number, pos_num, slot_id, HOST_ID };

            usb_write("piezo_elliptec_functions", bytesToSend, 6);
        }


        private void elliptec_clear_all_position_Click(object sender, RoutedEventArgs e)
        {
            if (cb_elliptec_select.SelectedIndex == -1)
                return;

            tb_pe_pos_save_1.Text = "2147483647";
            tb_pe_pos_save_2.Text = "2147483647";
        }

        private void elliptec_save_all_position_Click(object sender, RoutedEventArgs e)
        {
            if (cb_elliptec_select.SelectedIndex == -1)
                return;

            // get the stage selected from teh combo  box 
            byte slot = (byte)((Convert.ToByte(cb_elliptec_select.SelectedIndex) + 0x21) | 0x80);

            byte chan_id = (byte)(Convert.ToByte(cb_elliptec_select.SelectedIndex));

            // convert the um to encoder counts
            Double save_pos_1_t = Convert.ToDouble(tb_pe_pos_save_1.Text);
          //  save_pos_1_t *= 2.048;
            // check to make sure the value is not greater than 22 bits
            if (save_pos_1_t >= 2147483647)
                save_pos_1_t = 2147483647;
            byte[] save_pos_1 = BitConverter.GetBytes(Convert.ToInt32(save_pos_1_t));

            Double save_pos_2_t = Convert.ToDouble(tb_pe_pos_save_2.Text);
           // save_pos_2_t *= 2.048;
            // check to make sure the value is not greater than 22 bits
            if (save_pos_2_t >= 2147483647)
                save_pos_2_t = 2147483647;
            byte[] save_pos_2 = BitConverter.GetBytes(Convert.ToInt32(save_pos_2_t));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_SET_STORE_POSITION));
            byte[] bytesToSend = new byte[6 + 42] { command[0], command[1], 42, 0x00, slot, HOST_ID,
                chan_id, 0x00, 
                save_pos_1[0], save_pos_1[1], save_pos_1[2], save_pos_1[3],
                save_pos_2[0], save_pos_2[1], save_pos_2[2], save_pos_2[3],
                0,0,0,0, // not used
                0,0,0,0, // not used
                0,0,0,0, // not used
                0,0,0,0, // not used
                0,0,0,0, // not used
                0,0,0,0, // not used
                0,0,0,0, // not used
                0,0,0,0, // not used
             };
            usb_write("piezo_elliptec_functions", bytesToSend, 6 + 42);
        }

        private void elliptec_get_saved_positions()
        {
            byte slot = (byte)(ext_data[0] | ext_data[1]);
            double save_pos_1 = (ext_data[2] | ext_data[3] << 8 | ext_data[4] << 16 | ext_data[5] << 24);
            double save_pos_2 = (ext_data[6] | ext_data[7] << 8 | ext_data[8] << 16 | ext_data[9] << 24);
            double save_pos_3 = (ext_data[10] | ext_data[11] << 8 | ext_data[12] << 16 | ext_data[13] << 24);
            double save_pos_4 = (ext_data[14] | ext_data[15] << 8 | ext_data[16] << 16 | ext_data[17] << 24);
            double save_pos_5 = (ext_data[18] | ext_data[19] << 8 | ext_data[20] << 16 | ext_data[21] << 24);
            double save_pos_6 = (ext_data[22] | ext_data[23] << 8 | ext_data[24] << 16 | ext_data[25] << 24);

           // save_pos_1 /= 2.048;
           // save_pos_2 /= 2.048;

            this.Dispatcher.Invoke(new Action(() =>
            {
                tb_pe_pos_save_1.Text = save_pos_1.ToString("0,0.");
                tb_pe_pos_save_2.Text = save_pos_2.ToString("0,0.");
            }));
        }

        private void cb_elliptec_select_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // get the stage selected from the combo  box 
            byte slot = (byte)(Convert.ToByte(cb_elliptec_select.SelectedIndex) + 0x21);

            byte slot_num = (byte)(Convert.ToByte(cb_elliptec_select.SelectedIndex));

            // request Saved positions
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_REQ_STORE_POSITION));
            byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
            usb_write("piezo_elliptec_functions", bytesToSend, 6);
            Thread.Sleep(20);
        }

        private void cb_elliptec_select_SelectionChanged(object sender, RoutedEventArgs e)
        {
            cb_elliptec_select_SelectionChanged(sender, null);
        }

    }

}