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
        static int cycle_count = 0;
        byte pid_slot = 6;
        public void hexapod_update_request()
        {
            Byte slot_id = (Byte)((9) | 0x20);
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(0x0000)); ;
            if (cycle_count == 0)
            {
                command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_REQ_HEX_POSE));
                cycle_count = 1;
            }else if (cycle_count == 1)
            {
                slot_id = (Byte)(pid_slot | 0x20);
                command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_REQ_DCPIDPARAMS));
                cycle_count = 0;
            }

            byte[] bytesToSend = new byte[6] { command[0], command[1], 0x06, 0x00, slot_id, HOST_ID };
            usb_write("hexapod", bytesToSend, 6);
        }
        public void hexapod_update()
        {
            int command = header[0] | header[1] << 8;
            double temp, x, y, z, theta, phi, psi;
            int Kp, Ki, Kd, iMax;
            switch (command)
            {
                case MGMSG_MCM_GET_HEX_POSE:
                    temp = System.BitConverter.ToSingle(ext_data, 2);
                    x = Math.Round(temp, 3);
                    temp = System.BitConverter.ToSingle(ext_data, 6);
                    y = Math.Round(temp, 3);
                    temp = System.BitConverter.ToSingle(ext_data, 10);
                    z = Math.Round(temp, 3);
                    temp = System.BitConverter.ToSingle(ext_data, 14);
                    theta = Math.Round(temp, 3);
                    temp = System.BitConverter.ToSingle(ext_data, 18);
                    phi = Math.Round(temp, 3);
                    temp = System.BitConverter.ToSingle(ext_data, 22);
                    psi = Math.Round(temp, 3);


                    this.Dispatcher.Invoke(new Action(() =>
                    {
                        // temperature
                        tbStageX.Text = x.ToString("F3");
                        tbStageY.Text = y.ToString("F3");
                        tbStageZ.Text = z.ToString("F3");
                        tbStageTheta.Text = theta.ToString("F3");
                        tbStagePhi.Text = phi.ToString("F3");
                        tbStagePsi.Text = psi.ToString("F3");
                    }));
                    break;

                case MGMSG_MOT_GET_DCPIDPARAMS:
                    Kp = System.BitConverter.ToInt32(ext_data, 2);
                    Ki = System.BitConverter.ToInt32(ext_data, 6);
                    Kd = System.BitConverter.ToInt32(ext_data, 10);
                    iMax = System.BitConverter.ToInt32(ext_data, 14);
                    this.Dispatcher.Invoke(new Action(() =>
                    {
                        // temperature
                        lblKp.Content = Kp.ToString();
                        lblKi.Content = Ki.ToString();
                        lblKd.Content = Kd.ToString();
                        lbliMax.Content = iMax.ToString();
                    }));
                    break;
            }
            
        }
        public void btnKp_Click(object sender, RoutedEventArgs e)
        {
            Byte slot_id = (Byte)(pid_slot | 0x20);
            
            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_SET_DCPIDPARAMS));
            byte[] Kp = BitConverter.GetBytes(Convert.ToInt32(tbKp.Text));
            byte[] Ki = BitConverter.GetBytes(Convert.ToInt32(lblKi.Content));
            byte[] Kd = BitConverter.GetBytes(Convert.ToInt32(lblKd.Content));
            byte[] iMax = BitConverter.GetBytes(Convert.ToInt32(lbliMax.Content));
            byte[] bytesToSend;
            for (int c = 1; c <= 6; c++)
            {
                slot_id = (Byte)(c | 0x20);

                bytesToSend = new byte[26] { command[0], command[1], 20, 0, Convert.ToByte(slot_id | 0x80), HOST_ID,
                                                (byte)(pid_slot - 1), 0x00,
                                                Kp[0], Kp[1], Kp[2], Kp[3],
                                                Ki[0], Ki[1], Ki[2], Ki[3],
                                                Kd[0], Kd[1], Kd[2], Kd[3],
                                                iMax[0], iMax[1], iMax[2], iMax[3],
                                                0x00, 0x00};

                usb_write("hexapod", bytesToSend, 26);
            }
        }

        public void btnKi_Click(object sender, RoutedEventArgs e)
        {
            Byte slot_id = (Byte)(pid_slot | 0x20);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_SET_DCPIDPARAMS));
            byte[] Kp = BitConverter.GetBytes(Convert.ToInt32(lblKp.Content));
            byte[] Ki = BitConverter.GetBytes(Convert.ToInt32(tbKi.Text));
            byte[] Kd = BitConverter.GetBytes(Convert.ToInt32(lblKd.Content));
            byte[] iMax = BitConverter.GetBytes(Convert.ToInt32(lbliMax.Content));
            byte[] bytesToSend;
            for (int c = 1; c <= 6; c++)
            {
                slot_id = (Byte)(c | 0x20);

                bytesToSend = new byte[26] { command[0], command[1], 20, 0, Convert.ToByte(slot_id | 0x80), HOST_ID,
                                                (byte)(pid_slot - 1), 0x00,
                                                Kp[0], Kp[1], Kp[2], Kp[3],
                                                Ki[0], Ki[1], Ki[2], Ki[3],
                                                Kd[0], Kd[1], Kd[2], Kd[3],
                                                iMax[0], iMax[1], iMax[2], iMax[3],
                                                0x00, 0x00};

                usb_write("hexapod", bytesToSend, 26);
                Thread.Sleep(10);
            }
        }

        public void btnKd_Click(object sender, RoutedEventArgs e)
        {
            Byte slot_id = (Byte)(pid_slot | 0x20);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MOT_SET_DCPIDPARAMS));
            byte[] Kp = BitConverter.GetBytes(Convert.ToInt32(lblKp.Content));
            byte[] Ki = BitConverter.GetBytes(Convert.ToInt32(lblKi.Content));
            byte[] Kd = BitConverter.GetBytes(Convert.ToInt32(tbKd.Text));
            byte[] iMax = BitConverter.GetBytes(Convert.ToInt32(lbliMax.Content));
            byte[] bytesToSend;
            for (int c = 1; c <= 6; c++)
            {
                slot_id = (Byte)(c | 0x20);

                bytesToSend = new byte[26] { command[0], command[1], 20, 0, Convert.ToByte(slot_id | 0x80), HOST_ID,
                                                (byte)(pid_slot - 1), 0x00,
                                                Kp[0], Kp[1], Kp[2], Kp[3],
                                                Ki[0], Ki[1], Ki[2], Ki[3],
                                                Kd[0], Kd[1], Kd[2], Kd[3],
                                                iMax[0], iMax[1], iMax[2], iMax[3],
                                                0x00, 0x00};

                usb_write("hexapod", bytesToSend, 26);
                Thread.Sleep(10);
            }
        }

        private void btnHexJogUp_Click(object sender, RoutedEventArgs e)
        {
            Byte slot_id = (Byte)((8) | 0x20);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_HEX_POSE));
            byte[] x = BitConverter.GetBytes(0 + Convert.ToSingle(tbHexJog.Text));
            byte[] y = BitConverter.GetBytes(Convert.ToSingle(0));
            byte[] z = BitConverter.GetBytes(Convert.ToSingle(290));

            byte[] bytesToSend = new byte[26] { command[0], command[1], 20, 0, Convert.ToByte(slot_id | 0x80), HOST_ID,
                                                0x00, 0x00,
                                                x[0], x[1], x[2], x[3],
                                                y[0], y[1], y[2], y[3],
                                                z[0], z[1], z[2], z[3],
                                                0x00, 0x00, 0x00, 0x00,
                                                0x00, 0x00};

            usb_write("hexapod", bytesToSend, 26);
        }

        private void btnHexJogDwn_Click(object sender, RoutedEventArgs e)
        {
            Byte slot_id = (Byte)((8) | 0x20);

            byte[] command = BitConverter.GetBytes(Convert.ToInt32(MGMSG_MCM_SET_HEX_POSE));
            byte[] x = BitConverter.GetBytes(0 - Convert.ToSingle(tbHexJog.Text));
            byte[] y = BitConverter.GetBytes(Convert.ToSingle(0));
            byte[] z = BitConverter.GetBytes(Convert.ToSingle(290));

            byte[] bytesToSend = new byte[26] { command[0], command[1], 20, 0, Convert.ToByte(slot_id | 0x80), HOST_ID,
                                                0x00, 0x00,
                                                x[0], x[1], x[2], x[3],
                                                y[0], y[1], y[2], y[3],
                                                z[0], z[1], z[2], z[3],
                                                0x00, 0x00, 0x00, 0x00,
                                                0x00, 0x00};

            usb_write("hexapod", bytesToSend, 26);
        }
    }
}
