using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.IO;
using System.Windows.Forms.DataVisualization.Charting;

namespace SystemXMLParser
{
    public class MirrorChannelXML : IConfigurable
    {
        private Dictionary<ushort, byte[]> _data;

        public int Slot { get; }

        public int Channel { get; }


        public MirrorChannelXML()
        {
            _data = new Dictionary<ushort, byte[]>();
        }

        public MirrorChannelXML(string path_to_file, int slot, int channel)
        {
            Slot = slot;
            Channel = channel;
            XmlDocument doc = new XmlDocument();
            doc.Load(path_to_file);
            Utils.PreprocessingContext context = new();
            Utils.PathAsFileDir(path_to_file, context);
            Utils.PreprocessNode(doc, context);
            Setup(doc.DocumentElement);
        }

        private struct NodeQuery
        {
            public XmlNode PowerUpPosition = null;
            public XmlNode HighLevelIsINPosition = null;

            public readonly bool Complete =>
                PowerUpPosition is not null &&
                HighLevelIsINPosition is not null;


            public NodeQuery() { }
        }
        private NodeQuery GetParts(XmlElement element)
        {
            return new NodeQuery()
            {
                PowerUpPosition = element.SelectSingleNode("/Shutter/PowerUpPosition"),
                HighLevelIsINPosition = element.SelectSingleNode("/Shutters/PowerUpPosition"),
            };
        }

        private void Setup(object parameter)
        {
            _data = new();

            var query = GetParts(parameter as XmlElement);

            throw new NotImplementedException();
            // TODO:  Generate command intermediaries

            // TODO:  Add commands for commit
        }

        public void Configure(ConfigurationParams args)
        {
            foreach (KeyValuePair<ushort, byte[]> kvp in _data)
            {
                args.usb_send(kvp.Value);
            }
        }

        /// <summary>
        /// If save to eeprom is true, then all the commands will also be saved to EEPROM.
        /// </summary>
        public void Configure(ConfigurationParams args, bool save_to_eeprom)
        {
            Configure(args);

            // Save to EEPROM if flagged.
            if (save_to_eeprom)
            {
                byte[] command = { 0, 0, 4, 0, (byte)(Thorlabs.APT.Address.SLOT_1 + (Slot - 1) | 0x80), Thorlabs.APT.Address.HOST, 0, 0, 0, 0 };
                Array.Copy(BitConverter.GetBytes(Thorlabs.APT.MGMSG_MOT_SET_EEPROMPARAMS), 0, command, 0, 2);

                foreach (KeyValuePair<ushort, byte[]> kvp in _data)
                {
                    Array.Copy(BitConverter.GetBytes(kvp.Key), 0, command, 8, 2);
                    args.usb_send(command);
                }
            }
        }
    }
}
