using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.IO;
using MCM6000_UI;

namespace SystemXMLParser
{
    public struct ConfigurationParams
    {
        public delegate void USBSend(byte[] data);
        public delegate void USBSendLen(byte[] data, int length);
        public delegate byte[] USBResponse();
        public delegate byte[]? USBResponseTimeout(int ms_timeout);
        public delegate void RestartBoard();


        public USBSend usb_send;
        public USBSendLen usb_send_fragment;
        public USBResponse usb_response;
        public USBResponseTimeout usb_timeout;
        public RestartBoard restart_board;
        public int DEFAULT_TIMEOUT;
        public EFSDriver efs_driver;
    }


    public partial class SystemXML
    {
        private Settings? _settings;
        private AllowedDevices? _allowed_devices;
        private Joysticks? _joysticks;

        public Settings? XmlSettings { get { return _settings; } }
        public AllowedDevices? XmlAllowedDevices { get { return _allowed_devices; } }
        public Joysticks? XmlJoysticks { get { return _joysticks; } }


        public SystemXML(string path_to_file)
        {
            Setup(Utils.OpenXml(path_to_file));
        }

        private void Setup(XmlElement element)
        {
            // Slots are 1 indexed here (b/c i made poor decisions :)  )
            foreach (XmlNode subn in element.ChildNodes)
            {
                if (subn is XmlElement sub)
                {
                    switch (sub.Name)
                    {
                        case "Settings":
                            _settings = new(sub);
                            break;
                        case "AllowedDevices":
                            _allowed_devices = new(sub);
                            break;
                        case "Joysticks":
                            _joysticks = new(sub);
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }
}
