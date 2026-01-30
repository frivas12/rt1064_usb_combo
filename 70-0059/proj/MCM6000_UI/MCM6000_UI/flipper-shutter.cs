using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MCM6000_UI
{
    public partial class MainWindow
    {
        private void flipper_shutter_get_interlock_state()
        { }
        private void flipper_shutter_get_mirror_state()
        {
            var args = DeserializeMirrorState(header);

            foreach (var win in flipper_shutter_windows)
            { win.HandleEvent(args); }
        }

        private controls.flipper_shutter.ChannelEnableEventArgs DeserializeChannelEnable(byte[] data)
        {
            return new()
            {
                slot = (byte)(data[5] - Thorlabs.APT.Address.SLOT_1),
                channel = data[2],
                is_enabled = data[3] != 0,
            };
        }
        private void flipper_shutter_get_channel_enable()
        {
            var args = DeserializeChannelEnable(header);

            foreach (var win in flipper_shutter_windows)
            { win.HandleEvent(args); }
        }
    }
}
