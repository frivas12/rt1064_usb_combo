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

namespace MCM6000_UI
{
    public partial class MainWindow
    {
        private void send_save_command(byte slot, byte[] command)
        {
            slot |= 0x80;
            byte[] bytesToSend = new byte[10] { 0, 0, 4, 0, slot, Thorlabs.APT.Address.HOST, 0, 0, 0, 0 };
            Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOT_SET_EEPROMPARAMS), 0, bytesToSend, 0, 2);
            Array.Copy(command, 0, bytesToSend, 8, 2);

            usb_write("stepper_params", bytesToSend, 10);
        }

        private void stepper_params_refresh_Click(object sender, RoutedEventArgs e)
        {
            cb_stepper_select_SelectionChanged(sender, null);
        }

        private void Clear_soft_limits_Click(object sender, RoutedEventArgs e)
        {
            tbCW_soft_limit.Text = "2147483647";
            tbCCW_soft_limit.Text = "-2147483648";
        }

        private void stepper_initialize(int slot_num)
        {
            byte slot = (byte)(slot_num + 0x21);

            // request limit params
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_MOT_REQ_LIMSWITCHPARAMS));
            byte[] bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
            usb_write("stepper_params", bytesToSend, 6);
            Thread.Sleep(50);

            // request home params
            command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_HOMEPARAMS));
            bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
            usb_write("stepper_params", bytesToSend, 6);
            Thread.Sleep(10);

            // request Stepper drive params
            command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_STAGEPARAMS));
            bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
            usb_write("stepper_params", bytesToSend, 6);
            Thread.Sleep(10);

            // request PID params
            command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_REQ_DCPIDPARAMS));
            bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
            usb_write("stepper_params", bytesToSend, 6);
            Thread.Sleep(20);

            // request Saved positions
            command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_REQ_STORE_POSITION));
            bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
            usb_write("stepper_params", bytesToSend, 6);
            Thread.Sleep(20);

            // request Saved positions deadband
            command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_REQ_STORE_POSITION_DEADBAND));
            bytesToSend = new byte[6] { command[0], command[1], 0x00, 0x00, slot, HOST_ID };
            usb_write("stepper_params", bytesToSend, 6);
            Thread.Sleep(20);

            _stepper_initialized[slot_num] = true;
        }

        private void cb_stepper_select_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // get the stage selected from the combo  box 
            byte slot_num = (byte)(Convert.ToByte(cb_stage_select.SelectedIndex));


            // string[] card_types = Enum.GetNames(typeof(CardTypes));
            //int[] card_t = (int) (Enum.GetValues(typeof(CardTypes)));

            stepper_initialize(slot_num);

        }

        private void btn_set_abs_low_lim_Click(object sender, RoutedEventArgs e)
        {
            // get the stage selected from teh combo  box 
            byte slot = (byte)((Convert.ToByte(cb_stage_select.SelectedIndex) + 0x21));
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_ABS_LIMITS));
            byte[] bytesToSend = new byte[6] { command[0], command[1], 1, 0x00, slot, HOST_ID };

            usb_write("stepper_params", bytesToSend, 6);
        }

        private void btn_set_abs_high_lim_Click(object sender, RoutedEventArgs e)
        {
            // get the stage selected from teh combo  box 
            byte slot = (byte)((Convert.ToByte(cb_stage_select.SelectedIndex) + 0x21));
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_ABS_LIMITS));
            byte[] bytesToSend = new byte[6] { command[0], command[1], 2, 0x00, slot, HOST_ID };

            usb_write("stepper_params", bytesToSend, 6);
        }

        private void btn_clear_abs_lim_Click(object sender, RoutedEventArgs e)
        {
            // get the stage selected from teh combo  box 
            byte slot = (byte)((Convert.ToByte(cb_stage_select.SelectedIndex) + 0x21));
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_ABS_LIMITS));
            byte[] bytesToSend = new byte[6] { command[0], command[1], 3, 0x00, slot, HOST_ID };

            usb_write("stepper_params", bytesToSend, 6);
        }

        private void stepper_get_limits_params()
        {
            byte slot = (byte)(ext_data[0] | ext_data[1]);

            // CW Hardlimit
            Int16 cw_h_lim = (Int16)(ext_data[2] | ext_data[3] << 8);
            cw_hardlimit_save[slot] = cw_h_lim;

            // CCW Hardlimit
            Int16 ccw_h_lim = (Int16)(ext_data[4] | ext_data[5] << 8);
            ccw_hardlimit_save[slot] = ccw_h_lim;

            // CW Softlimit
            Int32 cw_s_l = (ext_data[6] | ext_data[7] << 8 | ext_data[8] << 16 | ext_data[9] << 24);

            // CCW Softlimit
            Int32 ccw_s_l = (ext_data[10] | ext_data[11] << 8 | ext_data[12] << 16 | ext_data[13] << 24);

            // hi abs limit
            Int32 abs_hi_limit = (ext_data[14] | ext_data[15] << 8 | ext_data[16] << 16 | ext_data[17] << 24);

            // low abs limit
            Int32 abs_low_limit = (ext_data[18] | ext_data[19] << 8 | ext_data[20] << 16 | ext_data[21] << 24);

            // Limit Mode
            Int16 lim_mode = (Int16)(ext_data[22] | ext_data[23] << 8);
            limit_mode_save[slot] = (Int16)(lim_mode);

            this.Dispatcher.Invoke(new Action(() =>
            {
                // cw swap
                if ((cw_h_lim & 0x80) == 0x80)
                {
                    Limit_swap.IsChecked = true;
                }
                else
                {
                    Limit_swap.IsChecked = false;
                }

                // cw limit 
                cw_h_lim &= 0x7F;
                cb_cw_hardlimit.SelectedIndex = cw_h_lim;

                // ccw swap
                if ((ccw_h_lim & 0x80) == 0x80)
                {
                    Limit_swap.IsChecked = true;
                }
                else
                {
                    Limit_swap.IsChecked = false;
                }

                limit_mode.SelectedIndex = lim_mode;

                // ccw limit 
                ccw_h_lim &= 0x7F;
                cb_ccw_hardlimit.SelectedIndex = ccw_h_lim;

                // soft limits
                tbCW_soft_limit.Text = cw_s_l.ToString();
                tbCCW_soft_limit.Text = ccw_s_l.ToString();

                // abs encoder limits
                tb_abs_high_limit.Text = abs_hi_limit.ToString();
                tb_abs_low_limit.Text = abs_low_limit.ToString();
            }));
        }

        private void Limits_save_Click(object sender, RoutedEventArgs e)
        {
            // get the stage selected from teh combo  box 
            byte slot = (byte)((Convert.ToByte(cb_stage_select.SelectedIndex) + 0x21) | 0x80);

            byte chan_id = (byte)(Convert.ToByte(cb_stage_select.SelectedIndex));

            bool saving = e_cbx_limits_persistent.IsChecked == true;

            int cw_hardlimit_temp = 0;
            int ccw_hardlimit_temp = 0;

            // first row in GUI
            cw_hardlimit_temp |= cb_cw_hardlimit.SelectedIndex;
            ccw_hardlimit_temp |= cb_ccw_hardlimit.SelectedIndex;
            Int16 limit_mode_temp = (Int16)(limit_mode.SelectedIndex);
            // limit swap
            if (Limit_swap.IsChecked == true)
            {
                cw_hardlimit_temp |= 0x80;
                ccw_hardlimit_temp |= 0x80;
            }

            // 2nd row in GUI
            int cw_softlimit_temp = int.Parse(tbCW_soft_limit.Text);
            int ccw_softlimit_temp = int.Parse(tbCCW_soft_limit.Text);

            // 3rd row in GUI
            int abs_hi_limit_temp = int.Parse(tb_abs_high_limit.Text);
            int abs_low_limit_temp = int.Parse(tb_abs_low_limit.Text);

            //write limit switch params 22-6 = 16 bytes
            byte[] cw_hardlimit = BitConverter.GetBytes(cw_hardlimit_temp);
            byte[] ccw_hardlimit = BitConverter.GetBytes(ccw_hardlimit_temp);
            byte[] cw_softlimit_t = BitConverter.GetBytes(cw_softlimit_temp);
            byte[] ccw_softlimit_t = BitConverter.GetBytes(ccw_softlimit_temp);
            byte[] abs_hi_limit_t = BitConverter.GetBytes(abs_hi_limit_temp);
            byte[] abs_low_limit_t = BitConverter.GetBytes(abs_low_limit_temp);
            byte[] limit_mode_t = BitConverter.GetBytes(limit_mode_temp);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_MOT_SET_LIMSWITCHPARAMS));
            byte[] bytesToSend = new byte[30] { command[0], command[1], 24, 0x00, slot, HOST_ID,
                chan_id, 0x00,
                cw_hardlimit[0], cw_hardlimit[1],
                ccw_hardlimit[0], ccw_hardlimit[1],
                cw_softlimit_t[0], cw_softlimit_t[1], cw_softlimit_t[2], cw_softlimit_t[3],
                ccw_softlimit_t[0], ccw_softlimit_t[1], ccw_softlimit_t[2], ccw_softlimit_t[3],
                abs_hi_limit_t[0], abs_hi_limit_t[1], abs_hi_limit_t[2], abs_hi_limit_t[3],
                abs_low_limit_t[0], abs_low_limit_t[1], abs_low_limit_t[2], abs_low_limit_t[3],
                limit_mode_t[0], limit_mode_t[1]
                };
            usb_write("stepper_params", bytesToSend, 30);
            if (saving)
            {
                Thread.Sleep(20);
                send_save_command(slot, command);
            }
        }

        private void stepper_get_home_params()
        {
            byte slot = (byte)(ext_data[0] | ext_data[1]);

            // Home Mode
            byte home_mode = ext_data[2];

            // Home Dir
            byte home_dir = ext_data[3];

            // Limit Switch
            Int16 limit_switch = (Int16)(ext_data[4] | ext_data[5] << 8);

            // Home Velocity
            Int32 home_vel = (ext_data[6] | ext_data[7] << 8 | ext_data[8] << 16 | ext_data[9] << 24);

            // Offset Distance
            Double offset = (ext_data[10] | ext_data[11] << 8 | ext_data[12] << 16 | ext_data[13] << 24);
            offset *= slot_nm_per_count[slot];
            offset /= 1e3;

            this.Dispatcher.Invoke(new Action(() =>
            {

                // home direction
                cb_home_mode.SelectedIndex = home_mode;
                cb_home_dir.SelectedIndex = home_dir;
                cb_home_limit.SelectedIndex = limit_switch;
                Home_vel.Text = home_vel.ToString();
                tb_home_offset.Text = offset.ToString();
            }));

        }

        private void Home_params_save_Click(object sender, RoutedEventArgs e)
        {
            // get the stage selected from teh combo  box 
            byte slot = (byte)((Convert.ToByte(cb_stage_select.SelectedIndex) + 0x21) | 0x80);

            byte chan_id = (byte)(Convert.ToByte(cb_stage_select.SelectedIndex));

            bool saving = e_cbx_home_persistent.IsChecked == true;

            byte home_mode = Convert.ToByte(cb_home_mode.SelectedIndex);
            byte home_dir = Convert.ToByte(cb_home_dir.SelectedIndex);
            Int16 limit_switch_t = Convert.ToInt16(cb_home_limit.SelectedIndex);
            Int32 home_velocity_t = Int32.Parse(Home_vel.Text);

            //write home params 20-6 = 14 bytes
            byte[] limit_switch = BitConverter.GetBytes(limit_switch_t);
            byte[] home_velocity = BitConverter.GetBytes(home_velocity_t);

            // convert the um to encoder counts
            // encoder counts = (Xum*1000)/nm_per_count
            Double offset_in_encoder_counts = Convert.ToDouble(tb_home_offset.Text);
            offset_in_encoder_counts *= 1e3;
            offset_in_encoder_counts /= slot_nm_per_count[chan_id];
            //Double offset_in_encoder_counts = (Convert.ToDouble(tb_home_offset.Text) * 1000) / slot_nm_per_count[chan_id];
            byte[] offset_distance = BitConverter.GetBytes(Convert.ToInt32(offset_in_encoder_counts));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_HOMEPARAMS));
            byte[] bytesToSend = new byte[20] { command[0], command[1], 14, 0x00, slot, HOST_ID,
                chan_id, 0x00,
                home_mode,
                home_dir,
                limit_switch[0], limit_switch[1],
                home_velocity[0], home_velocity[1], home_velocity[2], home_velocity[3],
                offset_distance[0], offset_distance[1], offset_distance[2], offset_distance[3]
             };
            usb_write("stepper_params", bytesToSend, 20);

            if (saving)
            {
                Thread.Sleep(20);
                send_save_command(slot, command);
            }
        }


        private void stepper_get_pid_params()
        {
            byte slot = (byte)(ext_data[0] | ext_data[1]);
            Int32 Kp = (ext_data[2] | ext_data[3] << 8 | ext_data[4] << 16 | ext_data[5] << 24);
            Int32 Ki = (ext_data[6] | ext_data[7] << 8 | ext_data[8] << 16 | ext_data[9] << 24);
            Int32 Kd = (ext_data[10] | ext_data[11] << 8 | ext_data[12] << 16 | ext_data[13] << 24);
            Int32 imax = (ext_data[14] | ext_data[15] << 8 | ext_data[16] << 16 | ext_data[17] << 24);

            // FilterControl in APT not used
            this.Dispatcher.Invoke(new Action(() =>
            {
                tb_p_term.Text = Kp.ToString();
                tb_i_term.Text = Ki.ToString();
                tb_d_term.Text = Kd.ToString();
                tb_imax.Text = imax.ToString();
                tb_p_term_s.Text = Kp.ToString();
                tb_i_term_s.Text = Ki.ToString();
                tb_d_term_s.Text = Kd.ToString();
                tb_imax_s.Text = imax.ToString();
            }));
        }

        private void stepper_get_drive_params()
        {
            byte slot = (byte)(ext_data[0] | ext_data[1]);
            char[] part_no_axis_t = new char[16] { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
            '\0', '\0', '\0', '\0', '\0', '\0', '\0' };

            Int16 stage_id = (Int16)(ext_data[2] | ext_data[3] << 8);
            Int16 axis_id = (Int16)(ext_data[4] | ext_data[5] << 8); // (not used)

            // Get the part number
            for (int i = 0; i < 16; i++)
            {
                if (ext_data[i + 6] != 0)
                    part_no_axis_t[i] = (char)ext_data[i + 6];
            }

            string part_no_string = new string(part_no_axis_t);
            part_no_string = part_no_string.Replace('\0'.ToString(), "");

            Int32 axis_serial_no = (ext_data[22] | ext_data[23] << 8 | ext_data[24] << 16 | ext_data[25] << 24);

            Double counts_per_unit_t = (ext_data[26] | ext_data[27] << 8 | ext_data[28] << 16 | ext_data[29] << 24);

            counts_per_unit_t = Math.Ceiling(counts_per_unit_t / 100) * 100;

            slot_counts_per_unit[slot] = counts_per_unit_t;

            Int32 min_pos = (ext_data[30] | ext_data[31] << 8 | ext_data[32] << 16 | ext_data[33] << 24);
            Int32 max_pos = (ext_data[34] | ext_data[35] << 8 | ext_data[36] << 16 | ext_data[37] << 24);

            // Stepper_drive_params
            Int32 acc = (ext_data[38] | ext_data[39] << 8 | ext_data[40] << 16 | ext_data[41] << 24);
            Int32 dec = (ext_data[42] | ext_data[43] << 8 | ext_data[44] << 16 | ext_data[45] << 24);
            Int32 max_speed = (ext_data[46] | ext_data[47] << 8 | ext_data[48] << 16 | ext_data[49] << 24);
            Int16 min_speed = (Int16)(ext_data[50] | ext_data[51] << 8);
            Int16 fs_spd = (Int16)(ext_data[52] | ext_data[53] << 8);
            byte kval_hold = ext_data[54];
            byte kval_run = ext_data[55];
            byte kval_acc = ext_data[56];
            byte kval_dec = ext_data[57];
            Int16 int_speed = (Int16)(ext_data[58] | ext_data[59] << 8);
            byte stall_th = ext_data[60];
            byte st_slp = ext_data[61];
            byte fn_slp_acc = ext_data[62];
            byte fn_slp_dec = ext_data[63];
            byte ocd_th = ext_data[64];
            byte step_mode = ext_data[65];
            Int16 config = (Int16)(ext_data[66] | ext_data[67] << 8);
            slot_nm_per_count[slot] = Math.Round((Double)System.BitConverter.ToSingle(ext_data, 68), 6); // I don't know why it rounds to 4 decimal places, but okay.

            Int32 gate_config = (Int32)(ext_data[72] | ext_data[73] << 8 | ext_data[74] << 16);
            Int16 stepper_approach_vel = (Int16)(ext_data[75] | ext_data[76] << 8);
            Int16 stepper_deadband = (Int16)(ext_data[77] | ext_data[78] << 8);
            Int16 stepper_backlash = (Int16)(ext_data[79] | ext_data[80] << 8);
            byte kickout_time = ext_data[81];

            /*Flags_save*/
            byte flags = ext_data[82];

            /*collision_threshold */
            UInt16 collision_threshold = (UInt16)(ext_data[83] | ext_data[84] << 8);

            /*encoder type */
            byte encoder_type = ext_data[85];
            UInt16 index_delta_min = (UInt16)(ext_data[86] | ext_data[87] << 8);
            UInt16 index_delta_incr = (UInt16)(ext_data[88] | ext_data[89] << 8);

            this.Dispatcher.Invoke(new Action(() =>
            {
                tb_stage_id.Text = stage_id.ToString();
                tb_stage_name.Text = part_no_string;
                tb_counts_per_unit.Text = counts_per_unit_t.ToString("R");
                tb_min_pos.Text = min_pos.ToString();
                tb_max_pos.Text = max_pos.ToString();
                tb_acc.Text = acc.ToString();
                tb_dec.Text = dec.ToString();
                tb_max_speed.Text = max_speed.ToString();
                tb_min_speed.Text = min_speed.ToString();
                tb_fs_spd.Text = fs_spd.ToString();
                tb_kval_hold.Text = kval_hold.ToString();
                tb_kval_run.Text = kval_run.ToString();
                //tb_kval_acc.Text = kval_acc.ToString(); // (use same as kval_run)
                tb_int_speed.Text = int_speed.ToString();
                tb_stall_th.Text = stall_th.ToString();
                tb_st_slp.Text = st_slp.ToString();
                tb_fn_slp_acc.Text = fn_slp_acc.ToString();
                tb_fn_slp_dec.Text = fn_slp_dec.ToString();
                tb_ocd_th.Text = ocd_th.ToString();
                cb_step_mode.SelectedIndex = step_mode;
                tb_config.Text = config.ToString();
                tb_nm_per_count.Text = slot_nm_per_count[slot].ToString();
                tb_gate_config.Text = gate_config.ToString();
                tb_approach_vel.Text = stepper_approach_vel.ToString();
                tb_stepper_deadband.Text = stepper_deadband.ToString();
                tb_stepper_backlash.Text = stepper_backlash.ToString();
                tb_kickout_time.Text = kickout_time.ToString();

                // has encoder
                if ((flags & (1 << 0)) == (1 << 0))
                    cb_has_encoder.IsChecked = true;
                else
                    cb_has_encoder.IsChecked = false;
                // encoder reversed
                if ((flags & (1 << 1)) == (1 << 1))
                    cb_encoder_reverse.IsChecked = true;
                else
                    cb_encoder_reverse.IsChecked = false;
                // has index
                if ((flags & (1 << 2)) == (1 << 2))
                    cb_has_index.IsChecked = true;
                else
                    cb_has_index.IsChecked = false;
                // stepper reversed
                if ((flags & (1 << 3)) == (1 << 3))
                    cb_stepper_reverse.IsChecked = true;
                else
                    cb_stepper_reverse.IsChecked = false;

                // Use PID for goto and jog commands
                if ((flags & (1 << 4)) == (1 << 4))
                    cb_use_PID.IsChecked = true;
                else
                    cb_use_PID.IsChecked = false;

                // Use PID for kickout (this stops the PID controller when the destination is reached)
                if ((flags & (1 << 5)) == (1 << 5))
                    cb_PID_kickout.IsChecked = true;
                else
                    cb_PID_kickout.IsChecked = false;

                if ((flags & (1 << 6)) != 0)
                    cb_is_rotational.IsChecked = true;
                else
                    cb_is_rotational.IsChecked = false;

                cb_prefers_soft_stop.IsChecked = (flags & (1 << 7)) != 0;

                

                tb_collision_threshold.Text = collision_threshold.ToString();

                cb_encoder_type.SelectedIndex = encoder_type;
                tb_index_delta.Text = index_delta_min.ToString();
                tb_index_incr.Text = index_delta_incr.ToString();
            }));

            foreach (var v in stepper_windows)
            {
                if (v.Item2.Slot == slot)
                {
                    v.Item2.NMPerCount = slot_nm_per_count[slot];
                    v.Item2.CountsPerUnit = counts_per_unit_t / 1e5;
                    v.Item2.StepperReversed = (flags & (1 << 3)) == (1 << 3);
                    v.Item2.OptionalSpeedLimit.HardwareSpeedLimit = (uint)max_speed;
                    v.Item2.IsRotational = (flags & (1 << 6)) != 0;
                    v.Item2.PrefersSoftStop = (flags & (1 << 7)) != 0;
                    v.Item2.OptionalSoftStopSupported = IsSoftStopFeatureAvailable();
                    v.Item1.PostConfiguration(v.Item2);
                }
            }
        }

        private void stepper_drive_params_save_Click(object sender, RoutedEventArgs e)
        {
            if (cb_stage_select.SelectedIndex == -1)
                return;

            // get the stage selected from the combo  box 
            byte slot = (byte)((Convert.ToByte(cb_stage_select.SelectedIndex) + 0x21) | 0x80);

            byte chan_id = (byte)(Convert.ToByte(cb_stage_select.SelectedIndex));

            bool saving = e_cbx_drive_persistent.IsChecked == true;

            Int16 stage_id_t = Convert.ToInt16(tb_stage_id.Text);
            Int16 axis_id_t = 0; // not used yet
            Int32 axis_serial_no_t = 0; // not used yet

            byte[] part_no_axis_t = new byte[16] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            byte[] part_no_axis_r = ASCIIEncoding.ASCII.GetBytes(tb_stage_name.Text);

            int length = tb_stage_name.Text.Length;
            for (int i = 0; i < length; i++)
            {
                part_no_axis_t[i] = part_no_axis_r[i];
            }

            //this.Dispatcher.Invoke(new Action(() =>
            //    {
            //        // control panel axis name
            //        cp_labels[chan_id, 0].Content = tb_stage_name.Text + ": ";

            //    }));

            // update the UI value for slot_nm_per_count
            slot_nm_per_count[cb_stage_select.SelectedIndex] = Convert.ToSingle(tb_nm_per_count.Text);
            // update the UI value for slot_counts_per_unit
            slot_counts_per_unit[cb_stage_select.SelectedIndex] = Convert.ToSingle(tb_counts_per_unit.Text);

            Int32 counts_per_unit_t = Convert.ToInt32(tb_counts_per_unit.Text);

            Int32 min_pos_t = Convert.ToInt32(tb_min_pos.Text);
            Int32 max_pos_t = Convert.ToInt32(tb_max_pos.Text);
            Int32 max_acc_t = Convert.ToInt32(tb_acc.Text);
            Int32 max_dec_t = Convert.ToInt32(tb_dec.Text);
            Int32 max_speed_t = Convert.ToInt32(tb_max_speed.Text);
            Int16 min_speed_t = Convert.ToInt16(tb_min_speed.Text);
            Int16 fs_spd_t = Convert.ToInt16(tb_fs_spd.Text);
            byte kval_hold_t = Convert.ToByte(tb_kval_hold.Text);
            byte kval_run_t = Convert.ToByte(tb_kval_run.Text);
            // kval_acc & kval_dec same a kval_run
            Int16 int_spd_t = Convert.ToInt16(tb_int_speed.Text);
            byte stall_th_t = Convert.ToByte(tb_stall_th.Text);
            byte st_slp_t = Convert.ToByte(tb_st_slp.Text);
            byte fn_slp_acc_t = Convert.ToByte(tb_fn_slp_acc.Text);
            byte fn_slp_dec_t = Convert.ToByte(tb_fn_slp_dec.Text);
            byte ocd_th_t = Convert.ToByte(tb_ocd_th.Text);
            byte step_mode_t = Convert.ToByte(cb_step_mode.SelectedIndex);
            Int16 config_t = Convert.ToInt16(tb_config.Text);
            Int32 gate_config_t = Convert.ToInt32(tb_gate_config.Text);
            Int16 stepper_approach_vel_t = Convert.ToInt16(tb_approach_vel.Text);
            Int16 stepper_deadband_t = Convert.ToInt16(tb_stepper_deadband.Text);
            Int16 stepper_backlash_t = Convert.ToInt16(tb_stepper_backlash.Text);
            byte kickout_time = Convert.ToByte(tb_kickout_time.Text);

            byte[] nm_per_count = BitConverter.GetBytes(Convert.ToSingle(tb_nm_per_count.Text));

            Double nm_per_count_t2 = Convert.ToDouble(tb_nm_per_count.Text);
            slot_nm_per_count[cb_stage_select.SelectedIndex] = nm_per_count_t2;
            byte flags_t = 0;

            // has encoder
            if (cb_has_encoder.IsChecked == true)
            {
                flags_t |= 1 << 0;
            }

            // encoder reversed
            if (cb_encoder_reverse.IsChecked == true)
            {
                flags_t |= 1 << 1;
            }

            // encoder has index
            if (cb_has_index.IsChecked == true)
            {
                flags_t |= 1 << 2;
            }

            // stepper reversed
            if (cb_stepper_reverse.IsChecked == true)
            {
                flags_t |= 1 << 3;
            }

            // Use PID controller for goto and jog commands
            if (cb_use_PID.IsChecked == true)
            {
                flags_t |= 1 << 4;
            }

            // Use PID for kickout (this stops the PID controller when the destination is reached)
            if (cb_PID_kickout.IsChecked == true)
            {
                flags_t |= 1 << 5;
            }

            // Is Rotational Stage
            if (cb_is_rotational.IsChecked == true)
            {
                flags_t |= 1 << 6;
            }

            // Prefers Soft Stop
            if (cb_prefers_soft_stop.IsChecked == true)
            {
                flags_t |= 1 << 7;
            }

            UInt16 collision_threshold_t = Convert.ToUInt16(tb_collision_threshold.Text);

            // encoder type
            byte encoder_type = (byte)(Convert.ToByte(cb_encoder_type.SelectedIndex));

            UInt16 index_delta_min_t = Convert.ToUInt16(tb_index_delta.Text);
            UInt16 index_delta_incr_t = Convert.ToUInt16(tb_index_incr.Text);


            byte[] stage_id = BitConverter.GetBytes(stage_id_t);
            byte[] axis_id = BitConverter.GetBytes(axis_id_t);
            byte[] axis_serial_no = BitConverter.GetBytes(axis_serial_no_t);
            byte[] counts_per_unit = BitConverter.GetBytes(counts_per_unit_t);
            byte[] min_pos = BitConverter.GetBytes(min_pos_t);
            byte[] max_pos = BitConverter.GetBytes(max_pos_t);
            byte[] max_acc = BitConverter.GetBytes(max_acc_t);
            byte[] max_dec = BitConverter.GetBytes(max_dec_t);
            byte[] max_speed = BitConverter.GetBytes(max_speed_t);
            byte[] min_speed = BitConverter.GetBytes(min_speed_t);
            byte[] fs_spd = BitConverter.GetBytes(fs_spd_t);
            byte[] int_spd = BitConverter.GetBytes(int_spd_t);

            byte[] config = BitConverter.GetBytes(config_t);
            byte[] gate_config = BitConverter.GetBytes(gate_config_t);
            byte[] stepper_approach_vel = BitConverter.GetBytes(stepper_approach_vel_t);
            byte[] stepper_deadband = BitConverter.GetBytes(stepper_deadband_t);
            byte[] stepper_backlash = BitConverter.GetBytes(stepper_backlash_t);

            byte[] collision_threshold = BitConverter.GetBytes(collision_threshold_t);

            byte[] index_delta_min = BitConverter.GetBytes(index_delta_min_t);
            byte[] index_delta_step = BitConverter.GetBytes(index_delta_incr_t);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_STAGEPARAMS));
            byte[] bytesToSend = new byte[6 + 96] { command[0], command[1], 96, 0x00, slot, HOST_ID,
                    chan_id, 0x00, 
                    // Stepper_Config
                    stage_id[0], stage_id[1], // 2 - 3
                    axis_id[0], axis_id[1],
                    part_no_axis_t[0], part_no_axis_t[1], part_no_axis_t[2], part_no_axis_t[3],
                    part_no_axis_t[4], part_no_axis_t[5], part_no_axis_t[6], part_no_axis_t[7],
                    part_no_axis_t[8], part_no_axis_t[9], part_no_axis_t[10], part_no_axis_t[11],
                    part_no_axis_t[12], part_no_axis_t[13], part_no_axis_t[14], part_no_axis_t[15],
                    axis_serial_no[0], axis_serial_no[1], axis_serial_no[2], axis_serial_no[3],
                    counts_per_unit[0], counts_per_unit[1], counts_per_unit[2], counts_per_unit[3], // 26 - 29
                    min_pos[0], min_pos[1], min_pos[2], min_pos[3],
                    max_pos[0], max_pos[1], max_pos[2], max_pos[3], 
                    // Stepper_drive_params
                    max_acc[0], max_acc[1], max_acc[2], max_acc[3], // 38 - 41
                    max_dec[0], max_dec[1], max_dec[2], max_dec[3],
                    max_speed[0], max_speed[1], max_speed[2], max_speed[3],
                    min_speed[0], min_speed[1],
                    fs_spd[0], fs_spd[1], //52 - 53
                    kval_hold_t, kval_run_t, kval_run_t, kval_run_t,
                    int_spd[0], int_spd[1],
                    stall_th_t,
                    st_slp_t,
                    fn_slp_acc_t,
                    fn_slp_dec_t,
                    ocd_th_t, // 64
                    step_mode_t,
                    config[0], config[1],
                    nm_per_count[0], nm_per_count[1], nm_per_count[2], nm_per_count[3],
                    gate_config[0], gate_config[1], gate_config[2], // 72 - 74 gatecfg1 (2 bytes) gatecfg2 (1 byte)
                    stepper_approach_vel[0], stepper_approach_vel[1],
                    stepper_deadband[0], stepper_deadband[1],
                    stepper_backlash[0], stepper_backlash[1],
                    kickout_time,
                    // Flags_save
                    flags_t, // 82
                    collision_threshold[0], collision_threshold[1], 
                    // Encoder_Save
                    encoder_type, // 85
                    index_delta_min[0], index_delta_min[1],
                    index_delta_step[0], index_delta_step[1],
                    0,0,0,0,0,0
                    };

            usb_write("stepper_params", bytesToSend, 6 + 96); // 102
            if (saving)
            {
                Thread.Sleep(20);
                send_save_command(slot, command);
            }

            controls.stepper.ConfigurationReadyArgs newConfig = new(cb_stage_select.SelectedIndex)
            {
                CountsPerUnit = counts_per_unit_t / 1e5,
                NMPerCount = nm_per_count_t2,
                StepperReversed = (flags_t & (1 << 3)) == (1 << 3),
        };

            foreach (var window in stepper_windows)
            {
                if (window.Item2.Slot == cb_stage_select.SelectedIndex)
                {
                    window.Item1.PostConfiguration(newConfig);
                }
            }
        }

        private void save_stepper_pid_params_Click(object sender, RoutedEventArgs e)
        {
            if (cb_stage_select.SelectedIndex == -1)
                return;

            // get the stage selected from the combo  box 
            byte slot_id = (byte)((Convert.ToByte(cb_stage_select.SelectedIndex) + 0x21) | 0x80);

            bool saving = e_cbx_pid_persistent.IsChecked == true;

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_SET_DCPIDPARAMS));
            byte[] Kp = BitConverter.GetBytes(Convert.ToInt32(tb_p_term.Text));
            byte[] Ki = BitConverter.GetBytes(Convert.ToInt32(tb_i_term.Text));
            byte[] Kd = BitConverter.GetBytes(Convert.ToInt32(tb_d_term.Text));
            byte[] iMax = BitConverter.GetBytes(Convert.ToInt32(tb_imax.Text));
            byte[] bytesToSend;

            bytesToSend = new byte[26] { command[0], command[1], 20, 0, Convert.ToByte(slot_id | 0x80), HOST_ID,
            (byte)(pid_slot - 1), 0x00,
            Kp[0], Kp[1], Kp[2], Kp[3],
            Ki[0], Ki[1], Ki[2], Ki[3],
            Kd[0], Kd[1], Kd[2], Kd[3],
            iMax[0], iMax[1], iMax[2], iMax[3],
            0x00, 0x00};

            usb_write("stepper_params", bytesToSend, 26);
            if (saving)
            {
                Thread.Sleep(20);
                send_save_command(slot_id, command);
            }
        }

        private void bt_copy_current_pos_GotFocus(object sender, RoutedEventArgs e)
        {
            if (cb_stage_select.SelectedIndex == -1)
                return;

            // get the stage selected from the combo  box 
            byte slot = (byte)(Convert.ToByte(cb_stage_select.SelectedIndex));

            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string save_chan_t = button_name.Substring(button_name.Length - 1, 1);
            byte save_chan = (byte)(Convert.ToByte(save_chan_t));

            TextBox tx_box = tb_pos_save_1;

            // get the current encoder counts for this slot
            stepper_update(slot);
        }

        private void bt_copy_current_pos_1_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            if (cb_stage_select.SelectedIndex == -1)
                return;


        }

        private void bt_copy_current_pos_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (cb_stage_select.SelectedIndex == -1)
                return;

            // on mouse down request current encoder position then on mouse up copy encoder position to textbox
            // in fuction bt_copy_current_pos_PreviewMouseLeftButtonUp below

            // get the stage selected from the combo  box 
            byte slot = (byte)(Convert.ToByte(cb_stage_select.SelectedIndex));

            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string save_chan_t = button_name.Substring(button_name.Length - 1, 1);
            byte save_chan = (byte)(Convert.ToByte(save_chan_t));

            TextBox tx_box = tb_pos_save_1;

            // get the current encoder counts for this slot
            stepper_update(slot);

        }

        private bool check_for_stepper_card(byte slot)
        {
            bool ret_val = false;

            switch (card_type[slot])
            {
                case (Int16)CardTypes.ST_Stepper_type:
                case (Int16)CardTypes.High_Current_Stepper_Card:
                case (Int16)CardTypes.High_Current_Stepper_Card_HD:
                case (Int16)CardTypes.ST_Invert_Stepper_BISS_type:
                case (Int16)CardTypes.ST_Invert_Stepper_SSI_type:
                case (Int16)CardTypes.MCM_Stepper_Internal_BISS_L6470:
                case (Int16)CardTypes.MCM_Stepper_Internal_SSI_L6470:
                case (Int16)CardTypes.MCM_Stepper_L6470_MicroDB15:
                case (Int16)CardTypes.MCM_Stepper_LC_HD_DB15:
                    ret_val = true;
                    break;
            }
            return ret_val;
        }

        private void bt_copy_current_pos_PreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            if (cb_stage_select.SelectedIndex == -1)
                return;

            // get the stage selected from the combo  box 
            byte slot = (byte)(Convert.ToByte(cb_stage_select.SelectedIndex));

            Button clickedButton = (Button)sender;
            string button_name = clickedButton.Name;
            string save_chan_t = button_name.Substring(button_name.Length - 1, 1);
            byte save_chan = (byte)(Convert.ToByte(save_chan_t));

            TextBox tx_box = tb_pos_save_1;

            switch (save_chan)
            {
                case 1:
                    tx_box = tb_pos_save_1;
                    break;

                case 2:
                    tx_box = tb_pos_save_2;
                    break;

                case 3:
                    tx_box = tb_pos_save_3;
                    break;

                case 4:
                    tx_box = tb_pos_save_4;
                    break;

                case 5:
                    tx_box = tb_pos_save_5;
                    break;

                case 6:
                    tx_box = tb_pos_save_6;
                    break;

                default:
                    break;
            }

            this.Dispatcher.Invoke(new Action(() =>
        {
            if (check_for_stepper_card(slot))
            {
                foreach (var window in stepper_windows)
                {
                    if (window.Item2.Slot == slot)
                    {
                        tx_box.Text = window.Item1.EncoderPosition;
                        break;
                    }
                }
            }
        }));

        }

        private void stepper_clear_all_position_Click(object sender, RoutedEventArgs e)
        {
            if (cb_stage_select.SelectedIndex == -1)
                return;

            tb_pos_save_1.Text = "2147483647";
            tb_pos_save_2.Text = "2147483647";
            tb_pos_save_3.Text = "2147483647";
            tb_pos_save_4.Text = "2147483647";
            tb_pos_save_5.Text = "2147483647";
            tb_pos_save_6.Text = "2147483647";

        }

        private void stepper_save_all_position_Click(object sender, RoutedEventArgs e)
        {
            if (cb_stage_select.SelectedIndex == -1)
                return;

            // get the stage selected from teh combo  box 
            byte slot = (byte)((Convert.ToByte(cb_stage_select.SelectedIndex) + 0x21) | 0x80);

            byte chan_id = (byte)(Convert.ToByte(cb_stage_select.SelectedIndex));

            // convert the um to encoder counts
            // encoder counts = (Xum*1000)/nm_per_count
            Double save_pos_1_t = Convert.ToDouble(tb_pos_save_1.Text);
            save_pos_1_t *= 1e3;
            save_pos_1_t /= slot_nm_per_count[chan_id];
            // check to make sure the value is not greater than 22 bits
            if (save_pos_1_t >= 2147483647)
                save_pos_1_t = 2147483647;
            byte[] save_pos_1 = BitConverter.GetBytes(Convert.ToInt32(save_pos_1_t));

            Double save_pos_2_t = Convert.ToDouble(tb_pos_save_2.Text);
            save_pos_2_t *= 1e3;
            save_pos_2_t /= slot_nm_per_count[chan_id];
            // check to make sure the value is not greater than 22 bits
            if (save_pos_2_t >= 2147483647)
                save_pos_2_t = 2147483647;
            byte[] save_pos_2 = BitConverter.GetBytes(Convert.ToInt32(save_pos_2_t));

            Double save_pos_3_t = Convert.ToDouble(tb_pos_save_3.Text);
            save_pos_3_t *= 1e3;
            save_pos_3_t /= slot_nm_per_count[chan_id];
            // check to make sure the value is not greater than 22 bits
            if (save_pos_3_t >= 2147483647)
                save_pos_3_t = 2147483647;
            byte[] save_pos_3 = BitConverter.GetBytes(Convert.ToInt32(save_pos_3_t));

            Double save_pos_4_t = Convert.ToDouble(tb_pos_save_4.Text);
            save_pos_4_t *= 1e3;
            save_pos_4_t /= slot_nm_per_count[chan_id];
            // check to make sure the value is not greater than 22 bits
            if (save_pos_4_t >= 2147483647)
                save_pos_4_t = 2147483647;
            byte[] save_pos_4 = BitConverter.GetBytes(Convert.ToInt32(save_pos_4_t));

            Double save_pos_5_t = Convert.ToDouble(tb_pos_save_5.Text);
            save_pos_5_t *= 1e3;
            save_pos_5_t /= slot_nm_per_count[chan_id];
            // check to make sure the value is not greater than 22 bits
            if (save_pos_5_t >= 2147483647)
                save_pos_5_t = 2147483647;
            byte[] save_pos_5 = BitConverter.GetBytes(Convert.ToInt32(save_pos_5_t));

            Double save_pos_6_t = Convert.ToDouble(tb_pos_save_6.Text);
            save_pos_6_t *= 1e3;
            save_pos_6_t /= slot_nm_per_count[chan_id];
            // check to make sure the value is not greater than 22 bits
            if (save_pos_6_t >= 2147483647)
                save_pos_6_t = 2147483647;
            byte[] save_pos_6 = BitConverter.GetBytes(Convert.ToInt32(save_pos_6_t));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_SET_STORE_POSITION));
            byte[] bytesToSend = new byte[6 + 42] { command[0], command[1], 42, 0x00, slot, HOST_ID,
                chan_id, 0x00,
                save_pos_1[0], save_pos_1[1], save_pos_1[2], save_pos_1[3],
                save_pos_2[0], save_pos_2[1], save_pos_2[2], save_pos_2[3],
                save_pos_3[0], save_pos_3[1], save_pos_3[2], save_pos_3[3],
                save_pos_4[0], save_pos_4[1], save_pos_4[2], save_pos_4[3],
                save_pos_5[0], save_pos_5[1], save_pos_5[2], save_pos_5[3],
                save_pos_6[0], save_pos_6[1], save_pos_6[2], save_pos_6[3],
                0,0,0,0, // not used
                0,0,0,0, // not used
                0,0,0,0, // not used
                0,0,0,0 // not used
             };
            usb_write("stepper_params", bytesToSend, 6 + 42);

            Thread.Sleep(20);
            stepper_set_saved_positions_deadband();
        }

        private void stepper_get_saved_positions()
        {
            byte slot = (byte)(ext_data[0] | ext_data[1]);
            int save_pos_1 = (ext_data[2] | ext_data[3] << 8 | ext_data[4] << 16 | ext_data[5] << 24);
            int save_pos_2 = (ext_data[6] | ext_data[7] << 8 | ext_data[8] << 16 | ext_data[9] << 24);
            int save_pos_3 = (ext_data[10] | ext_data[11] << 8 | ext_data[12] << 16 | ext_data[13] << 24);
            int save_pos_4 = (ext_data[14] | ext_data[15] << 8 | ext_data[16] << 16 | ext_data[17] << 24);
            int save_pos_5 = (ext_data[18] | ext_data[19] << 8 | ext_data[20] << 16 | ext_data[21] << 24);
            int save_pos_6 = (ext_data[22] | ext_data[23] << 8 | ext_data[24] << 16 | ext_data[25] << 24);
            int save_pos_7 = (ext_data[26] | ext_data[27] << 8 | ext_data[28] << 16 | ext_data[29] << 24);
            int save_pos_8 = (ext_data[30] | ext_data[31] << 8 | ext_data[32] << 16 | ext_data[33] << 24);
            int save_pos_9 = (ext_data[34] | ext_data[35] << 8 | ext_data[36] << 16 | ext_data[37] << 24);
            int save_pos_10 = (ext_data[38] | ext_data[39] << 8 | ext_data[40] << 16 | ext_data[41] << 24);

            // convert encoder counts to display in um
            Double save_pos_1_um = (save_pos_1 * slot_nm_per_count[slot]) / 1e3;
            Double save_pos_2_um = (save_pos_2 * slot_nm_per_count[slot]) / 1e3;
            Double save_pos_3_um = (save_pos_3 * slot_nm_per_count[slot]) / 1e3;
            Double save_pos_4_um = (save_pos_4 * slot_nm_per_count[slot]) / 1e3;
            Double save_pos_5_um = (save_pos_5 * slot_nm_per_count[slot]) / 1e3;
            Double save_pos_6_um = (save_pos_6 * slot_nm_per_count[slot]) / 1e3;

            this.Dispatcher.Invoke(new Action(() =>
            {
                tb_pos_save_1.Text = save_pos_1_um.ToString("0,0.000");
                tb_pos_save_2.Text = save_pos_2_um.ToString("0,0.000");
                tb_pos_save_3.Text = save_pos_3_um.ToString("0,0.000");
                tb_pos_save_4.Text = save_pos_4_um.ToString("0,0.000");
                tb_pos_save_5.Text = save_pos_5_um.ToString("0,0.000");
                tb_pos_save_6.Text = save_pos_6_um.ToString("0,0.000");
            }));

            foreach (var v in stepper_windows)
            {
                if (v.Item2.Slot == slot)
                {
                    int[] nums = new int[10] { save_pos_1, save_pos_2, save_pos_3, save_pos_4,
                        save_pos_5, save_pos_6, save_pos_7, save_pos_8, save_pos_9, save_pos_10 };
                    v.Item2.SavedPositions = nums;
                }
            }
        }
        private void stepper_get_saved_positions_deadband()
        {
            Int32 deadband = (ext_data[2] | ext_data[3] << 8);

            this.Dispatcher.Invoke(new Action(() =>
            {
                tb_position_deadband.Text = deadband.ToString();
            }));
        }

        private void stepper_set_saved_positions_deadband()
        {
            if (cb_stage_select.SelectedIndex == -1)
                return;

            // get the stage selected from teh combo  box 
            byte slot = (byte)((Convert.ToByte(cb_stage_select.SelectedIndex) + 0x21) | 0x80);

            byte chan_id = (byte)(Convert.ToByte(cb_stage_select.SelectedIndex));
            
            byte[] deadband = BitConverter.GetBytes(Convert.ToUInt16(tb_position_deadband.Text));

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_SET_STORE_POSITION_DEADBAND));
            byte[] bytesToSend = new byte[6 + 4] { command[0], command[1], 4, 0x00, slot, HOST_ID,
                chan_id, 0x00,
                deadband[0], deadband[1]
             };
            usb_write("stepper_params", bytesToSend, 6 + 4);
            Thread.Sleep(20);
        }

        private void tb_GotKeyboardFocus(object sender, KeyboardFocusChangedEventArgs e)
        {
            TextBox txtBox = sender as TextBox;
            txtBox.SelectAll();
        }

    }
}