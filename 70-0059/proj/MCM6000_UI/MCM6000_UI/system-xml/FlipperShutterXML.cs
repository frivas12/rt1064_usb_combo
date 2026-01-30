using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.IO;

namespace SystemXMLParser
{
    public class FlipperShutterXML : IPersistableConfigurable
    {
        private readonly List<IConfigurable> _children = [];

        public int Slot { get; }

        public FlipperShutterXML(XmlElement parent, int slot)
        {
            Slot = slot;

            Setup(parent);
        }

        public FlipperShutterXML(string path, int slot)
            : this(Utils.OpenXml(path).SelectSingleNode("/FlipperShutterParameters") as XmlElement, slot)
        { }


        private struct NodeQuery
        {
            public XmlNode Card = null;
            public IEnumerable<XmlNode> Shutters = [];
            public IEnumerable<XmlNode> Mirrors = [];

            public readonly bool Complete =>
                true;

            public NodeQuery() { }
        }
        private NodeQuery GetParts(XmlElement element)
        {
            NodeQuery rt = new()
            {
                Card = element.SelectSingleNode("Card"),
                Shutters = element.SelectNodes("Shutter").OfType<XmlNode>(),
                Mirrors = element.SelectNodes("Mirror").OfType<XmlNode>(),
            };

            return rt;
        }

        private void Setup(XmlElement element)
        {
            var query = GetParts(element);

            if (query.Complete)
            {
                // Cards Processing

                int shutterChannel = 0;
                foreach (XmlElement obj in query.Shutters.OfType<XmlElement>())
                {
                    int channel;
                    var channelNode = obj.Attributes.GetNamedItem("channel");
                    if (channelNode is not null)
                    {
                        channel = int.Parse(channelNode.Value);
                    }
                    else
                    {
                        channel = shutterChannel;
                    }
                    ++shutterChannel;
                    int version = int.Parse(obj.Attributes.GetNamedItem("version")?.Value ?? "-1");
                    IConfigurable configurator = version switch
                    {
                        -1 => throw new InvalidOperationException($"Shutter XML version was not specificied"),
                        0 => new ShutterChannelXML(obj, Slot, channel),
                        _ => throw new InvalidOperationException($"Shutter XML version \"{version}\" is unknown"),
                    };
                    _children.Add(configurator);
                }

                // Mirror Processing
            }

        }

        public void Configure(ConfigurationParams args) => Configure(args, false);

        /// <summary>
        /// If save to eeprom is true, then all the commands will also be saved to EEPROM.
        /// </summary>
        public void Configure(ConfigurationParams args, bool save_to_eeprom)
        {
            foreach (var child in _children)
            {
                if (child is IPersistableConfigurable cfg)
                {
                    cfg.Configure(args, save_to_eeprom);
                }
                else
                {
                    child.Configure(args);
                }
            }
        }
    }
}
