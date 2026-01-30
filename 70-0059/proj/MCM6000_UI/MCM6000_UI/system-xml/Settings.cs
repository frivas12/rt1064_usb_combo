using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reactive.Subjects;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace SystemXMLParser
{
    public partial class SystemXML
    {
        public class Settings : IConfigurable
        {
            internal class Card : ICollideable
            {
                // \todo Update CardType to the proper type. 

                // The slot the card goes in.
                internal int Slot { get; set; }

                // The type of card.
                internal ushort CardType { get; set; }

                internal bool Static { get; set; }

                internal bool IsDeviceDetectionEnabled { get; set; }
                
                internal IConfigurable CustomConfig
                { get; set; }

                internal Card()
                {
                    Slot = -1;
                    CardType = 0xFFFF;
                    Static = false;
                    IsDeviceDetectionEnabled = true;
                    CustomConfig = null;
                }

                internal Card(int slot, ushort cardType)
                {
                    Slot = slot;
                    CardType = cardType;
                    Static = false;
                    IsDeviceDetectionEnabled = true;
                    CustomConfig = null;
                }

                internal Card(XmlElement element, ref int automatic_index)
                {
                    string slot_str = element.GetAttribute("slot");
                    string card_type_str = element.GetAttribute("type");
                    string static_str = element.GetAttribute("static");

                    if (slot_str == "")
                    {
                        Slot = automatic_index;
                        ++automatic_index;
                    }
                    else
                    {
                        Slot = int.Parse(slot_str);
                        automatic_index = Slot + 1;
                    }

                    CardType = ushort.Parse(card_type_str);
                    Static = static_str == "1";
                    if (element.HasAttribute("device-detect"))
                    {
                        IsDeviceDetectionEnabled = element.GetAttribute("device-detect").ToLowerInvariant() == "true";
                    }
                    else
                    {
                        IsDeviceDetectionEnabled = true;
                    }

                    foreach (XmlNode child in element.ChildNodes)
                    {
                        if (child.Name == "CustomConfig" && child is XmlElement sub)
                        {
                            if (sub.ChildNodes.Count != 1)
                            {
                                throw new XmlException("<CustomConfig> must have only 1 child.");
                            }
                            if (sub.FirstChild is not XmlElement customElem)
                            {
                                throw new XmlException("The child of <CustomConfig> is not an XmlElement");
                            }
                            CustomConfig = customElem.Name switch
                            {
                                "StageParameters" => new StepperXML(customElem, Slot),
                                "FlipperShutterParameters" => new FlipperShutterXML(customElem, Slot),
                                _ => throw new XmlException($"custom slot config does not support config type of \"{customElem.Name}\""),
                            };
                        }
                    }
                }

                public bool Collides(ICollideable other)
                {
                    return other.GetType() == typeof(Card) && Slot == ((Card)other).Slot;
                }
            }

            private Card[] _cards;

            public Settings()
            {
                _cards = Array.Empty<Card>();
            }

            public Settings(string path_to_file)
            {
                XmlDocument doc = new XmlDocument();
                doc.Load(path_to_file);
                Utils.PreprocessingContext context = new();
                Utils.PathAsFileDir(path_to_file, context);
                Utils.PreprocessNode(doc, context);
                Setup(doc.DocumentElement);
            }

            internal Settings(XmlElement element)
            {
                Setup(element);
            }

            private void Setup(XmlElement element)
            {
                List<Card> cards = new List<Card>();
                int auto_counter = 1;
                foreach (XmlNode sub in element.ChildNodes)
                {
                    if (sub.Name == "Card" && sub is XmlElement subby)
                    {
                        cards.Add(new Card(subby, ref auto_counter));
                    }
                }

                Utils.ResolveCollisions(cards, Utils.ResolutionMode.ERROR, null);

                _cards = cards.ToArray();
            }

            /**
             * Returns an array of card types that are in use.
             */
            public ushort[] CardsInUse()
            {
                HashSet<ushort> cards = new();
                foreach (Card card in _cards)
                {
                    cards.Add(card.CardType);
                }

                return cards.ToArray();
            }

            public void ConfigureCustomParams(ConfigurationParams args)
            {
                foreach (var config in _cards.Where(x => x.CustomConfig is not null).Select(x => x.CustomConfig))
                {
                    if (config is IPersistableConfigurable persistable)
                    {
                        persistable.Configure(args, true);
                    }
                    else
                    {
                        config.Configure(args);
                    }
                }
            }

            public void Configure(ConfigurationParams args)
            {
                foreach (Card card in _cards)
                {
                    byte[] command = BitConverter.GetBytes(APT.MGMSG_SET_CARD_TYPE);
                    byte[] slot = BitConverter.GetBytes(card.Slot - 1);
                    byte[] card_type = BitConverter.GetBytes(card.CardType | (card.Static ? 0x8000 : 0x0000));

                    byte[] bytes_to_send = new byte[10]
                    {
                        command[0], command[1], 0x04, 0x00, 0x91, 0x01,
                        slot[0], slot[1], card_type[0], card_type[1]
                    };

                    args.usb_send(bytes_to_send);
                }

                foreach (var card in _cards)
                {
                    byte[] command = BitConverter.GetBytes(Convert.ToUInt16(APT.MGMSG_MCM_SET_DEVICE_DETECTION));
                    byte[] bytesToSend = new byte[9] {
                        command[0],
                        command[1],
                        0x03,
                        0x00,
                        0x91,
                        0x01,
                        (byte)(card.Slot - 1),
                        0x00,
                        (byte)(card.IsDeviceDetectionEnabled ? 0x01 : 0x00)
                    };

                    args.usb_send(bytesToSend);
                }
            }
        }
    }
}
