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

        /**************************************************************************************************************************************
        *  OTM default params
        * ************************************************************************************************************************************/

        private void Set_OTM_params()
        {

            //////////////////////////////////////////////////////////////////////////////////////////////////////
            // **** Slot 1 (X Axis) *****
            //////////////////////////////////////////////////////////////////////////////////////////////////////
            //////////////////////////////////////////////////////////////////////////////////////////////////////

            byte slot = (byte)((0 + 0x21) | 0x80);
            byte chan_id = (byte)(Convert.ToByte(0));

            // ***** Limits *****
            //write limit switch params 22-6 = 16 bytes
            byte[] cw_hardlimit = BitConverter.GetBytes(1);
            byte[] ccw_hardlimit = BitConverter.GetBytes(1);
            byte[] cw_softlimit_t = BitConverter.GetBytes(2147483647);
            byte[] ccw_softlimit_t = BitConverter.GetBytes(-2147483648);
            byte[] limit_mode_t = BitConverter.GetBytes(1);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_SET_LIMSWITCHPARAMS));
            byte[] bytesToSend = new byte[22] { command[0], command[1], 16, 0x00, slot, HOST_ID,
                                               chan_id, 0x00, cw_hardlimit[0], cw_hardlimit[1], ccw_hardlimit[0], ccw_hardlimit[1], 
                                               cw_softlimit_t[0], cw_softlimit_t[1], cw_softlimit_t[2], cw_softlimit_t[3], ccw_softlimit_t[0], ccw_softlimit_t[1], 
                                               ccw_softlimit_t[2], ccw_softlimit_t[3], limit_mode_t[0], limit_mode_t[1]
                                               };
            usb_write("otm-defaults", bytesToSend, 22);
            Thread.Sleep(50);

            // ***** Home params *****
            //write home params 20-6 = 14 bytes
            byte[] home_dir = BitConverter.GetBytes(2);
            byte[] limit_switch = BitConverter.GetBytes(3);
            byte[] home_velocity = BitConverter.GetBytes(100);

            // convert the um to encoder counts
            // encoder counts = (Xum*1000)/nm_per_count
            Double offset_in_encoder_counts = 0;
            if (slot_nm_per_count[chan_id] != 0)
                offset_in_encoder_counts = (Convert.ToDouble(1500) * 1000) / slot_nm_per_count[chan_id];
            byte[] offset_distance = BitConverter.GetBytes(Convert.ToInt32(offset_in_encoder_counts));

            command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_HOMEPARAMS));
            bytesToSend = new byte[20] { command[0], command[1], 14, 0x00, slot, HOST_ID,
                                          chan_id, 0x00, home_dir[0], home_dir[1], limit_switch[0], limit_switch[1],
                                          home_velocity[0], home_velocity[1], home_velocity[2], home_velocity[3], offset_distance[0], offset_distance[1],
                                          offset_distance[2], offset_distance[3]
                                      };
            usb_write("otm-defaults", bytesToSend, 20);
            Thread.Sleep(50);

            //////////////////////////////////////////////////////////////////////////////////////////////////////

            // ***** Stepper Params *****
            byte[] part_no_axis_t = new byte[16] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            byte[] part_no_axis_r = ASCIIEncoding.ASCII.GetBytes("Focus");

            int length = part_no_axis_r.Length;
            for (int i = 0; i < length; i++)
            {
                part_no_axis_t[i] = part_no_axis_r[i];
            }

            byte encoder_t = 0;

            // has encoder
            encoder_t |= 1;

            byte[] stage_id = BitConverter.GetBytes(0); // not used
            byte[] axis_id = BitConverter.GetBytes(0); // not used
            byte[] axis_serial_no = BitConverter.GetBytes(0); // not used
            byte[] counts_per_unit = BitConverter.GetBytes(1777800);
            byte[] min_pos = BitConverter.GetBytes(0);  // not used
            byte[] max_pos = BitConverter.GetBytes(0);  // not used
            byte[] max_acc = BitConverter.GetBytes(800);
            byte[] max_dec = BitConverter.GetBytes(800);
            byte[] max_speed = BitConverter.GetBytes(15);
            byte[] min_speed = BitConverter.GetBytes(0);
            byte[] fs_spd = BitConverter.GetBytes(1023);
            byte kval_hold = Convert.ToByte(23);
            byte kval_run = Convert.ToByte(200);
            byte[] int_spd = BitConverter.GetBytes(6930);
            byte stall_th = Convert.ToByte(31);
            byte st_slp = Convert.ToByte(29);
            byte fn_slp_acc = Convert.ToByte(68);
            byte fn_slp_dec = Convert.ToByte(68);
            byte ocd_th = Convert.ToByte(31);
            byte step_mode = Convert.ToByte(7);
            float nm_per_count_f = 500;
            byte[] nm_per_count = BitConverter.GetBytes(Convert.ToSingle(nm_per_count_f));
            byte[] collision_threshold = BitConverter.GetBytes(10);

            // set globlal variable
            Double nm_per_count_t2 = Convert.ToDouble(nm_per_count_f);
            slot_nm_per_count[0] = nm_per_count_t2;

            byte[] config = BitConverter.GetBytes(6930);

            command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_STAGEPARAMS));

            bytesToSend = new byte[80] { command[0], command[1], 74, 0x00, slot, HOST_ID,
                    chan_id, 0x00, 
                    stage_id[0], stage_id[1], // 2 - 3
                    axis_id[0], axis_id[1],  
                    part_no_axis_t[0], part_no_axis_t[1], part_no_axis_t[2], 
                    part_no_axis_t[3], part_no_axis_t[4], part_no_axis_t[5], 
                    part_no_axis_t[6], part_no_axis_t[7], part_no_axis_t[8], 
                    part_no_axis_t[9], part_no_axis_t[10], part_no_axis_t[11], 
                    part_no_axis_t[12], part_no_axis_t[13], part_no_axis_t[14], 
                    part_no_axis_t[15], 
                    axis_serial_no[0], axis_serial_no[1], axis_serial_no[2], axis_serial_no[3], 
                    counts_per_unit[0], counts_per_unit[1], counts_per_unit[2], counts_per_unit[3], // 26 - 29
                    min_pos[0], min_pos[1], min_pos[2], min_pos[3],
                    max_pos[0], max_pos[1], max_pos[2], max_pos[3], 
                    max_acc[0], max_acc[1], max_acc[2], max_acc[3], // 38 - 41
                    max_dec[0], max_dec[1], max_dec[2], max_dec[3],  
                    max_speed[0], max_speed[1], max_speed[2], max_speed[3],  
                    min_speed[0], min_speed[1],  
                    fs_spd[0], fs_spd[1], //52 - 53
                    kval_hold, kval_run, kval_run, kval_run,
                    int_spd[0], int_spd[1], 
                    stall_th,
                    st_slp, 
                    fn_slp_acc, 
                    fn_slp_dec, 
                    ocd_th, // 64
                    step_mode, 
                    config[0], config[1], 
                     nm_per_count[0], nm_per_count[1], nm_per_count[2], nm_per_count[3],
                    encoder_t, //72
                    collision_threshold[0]
                    };

            usb_write("otm-defaults", bytesToSend, 80);
            Thread.Sleep(50);

#if true
            /////////////////////////////////////////////////////////////////////////////////////////////////
            // parameters for Shutter
            /////////////////////////////////////////////////////////////////////////////////////////////////
            // get the slot selected from the combo  box 
            byte slot_id = (byte)(SLOT2 + 0x21 | 0x80);

            chan_id = SLOT2;

            byte shutter_initial_state = 1;     // Closed
            byte shutter_mode = 0;              // Pulse
            byte external_trigger_mode = 0;     // disabled
            byte on_time = 1;
            Int32 duty_cycle_pulse = 1900;
            Int32 duty_cycle_hold = 0;

            byte[] duty_cycle_pulse_t = BitConverter.GetBytes(duty_cycle_pulse);
            byte[] duty_cycle_hold_t = BitConverter.GetBytes(duty_cycle_hold);

            command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_SHUTTERPARAMS));
            bytesToSend = new byte[20] { command[0], command[1], 14, 0x00, slot_id, HOST_ID,
                chan_id, 0x00, 
                shutter_initial_state,
                shutter_mode,
                external_trigger_mode,
                on_time,   
                duty_cycle_pulse_t[0], duty_cycle_pulse_t[1], duty_cycle_pulse_t[2], duty_cycle_pulse_t[3],
                duty_cycle_hold_t[0], duty_cycle_hold_t[1], duty_cycle_hold_t[2], duty_cycle_hold_t[3] 
                };

            usb_write("otm-defaults", bytesToSend, 20);
            Thread.Sleep(50);

            // same for slot 3 shutter
            bytesToSend[4] += 1;
            usb_write("otm-defaults", bytesToSend, 20);
            Thread.Sleep(50);
#endif

#if flase
            /////////////////////////////////////////////////////////////////////////////////////////////////
            // joystick mapping
            /////////////////////////////////////////////////////////////////////////////////////////////////
            int j = 0;
            for (byte _port = 0; _port < 7; _port++)
            {
                // get how many items we have in the list for this port
                int num_of_mapping_items = 3;
                byte length_of_extended_data = (byte)((9 * num_of_mapping_items) + 2);

                bytesToSend = new byte[6 + length_of_extended_data];

                // MGMSG_LA_SET_JOYSTICK_MAP
                command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_LA_SET_JOYSTICK_MAP));
                byte[] header = new byte[] { command[0], command[1], length_of_extended_data, 0, Convert.ToByte(MOTHERBOARD_ID | 0x80), HOST_ID };

                Array.Copy(header, 0, bytesToSend, 0, header.Length);
                j = header.Length;

                // add port number bytesToSend buffer
                bytesToSend[j++] = _port;   // byte 6

                // skip for the length_of_data in mapping eeprom page, we will add it below to the buffer
                j++;

                // add all the mapping data to the 
                // button 1->X, 2->Y, 3->Z, 4->R,
                for (int i = 0; i < num_of_mapping_items; i++)
                {
                    bytesToSend[j++] = (byte)(6 + i);    // control_number
                    bytesToSend[j++] = 0;               // vid
                    bytesToSend[j++] = 0;               // vid
                    bytesToSend[j++] = 0;               // pid
                    bytesToSend[j++] = 0;               // pid
                    bytesToSend[j++] = 9;               // type
                    if (i >= 1)
                    {
                        bytesToSend[j++] = 3;               // mode Toggle Position
                        bytesToSend[j++] = (byte)(1 << i); //slot_receiver
                    }
                    else
                    {
                        bytesToSend[j++] = 4;               // mode Slot Select
                        bytesToSend[j++] = (byte)(1 << i); //slot_receiver
                    }
                    bytesToSend[j++] = 0;               // port_receiver
                }

                // add length_of_data bytesToSend buffer
                bytesToSend[7] = (byte)(j-6);  // byte 7/
                usb_write("otm-defaults", bytesToSend, j);
                Thread.Sleep(50);
            }
#endif
            /////////////////////////////////////////////////////////////////////////////////////////////////
            // resart uC
            /////////////////////////////////////////////////////////////////////////////////////////////////
            Restart_processor( true);

        }
    }


}