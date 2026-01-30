using ReactiveUI;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MCM6000_UI.builders
{
    public abstract class OneWireBuilderViewModel : ReactiveObject, IDisposable
    {
        public string ModeKey
        { get; }

        private string _deviceKey = string.Empty;
        public string DeviceKey
        {
            get => _deviceKey;
            protected set => this.RaiseAndSetIfChanged(ref _deviceKey, value);
        }

        private bool _isReady = false;
        public bool IsReady
        {
            get => _isReady;
            protected set => this.RaiseAndSetIfChanged(ref _isReady, value);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        public abstract void ConfigurePayload(byte[] payload, int offset, int length);

        protected OneWireBuilderViewModel(string modeKey)
        {
            ModeKey = modeKey;
        }

        protected virtual void Dispose(bool disposing)
        { }

        /// <summary>
        /// Renames the "Part Number" field in the one-wire binary payload.
        /// </summary>
        /// <exception cref="ArgumentNullException"></exception>
        /// <exception cref="ArgumentOutOfRangeException"></exception>
        /// <exception cref="ArgumentException"></exception>
        protected static void RenamePayload(byte[] buffer, int offset, int length, string name)
        {
            if (buffer is null)
            { throw new ArgumentNullException(nameof(buffer)); }
            if (name is null)
            { throw new ArgumentNullException(nameof(name)); }
            if (offset < 0)
            { throw new ArgumentOutOfRangeException(nameof(offset), "offset must be non-negative"); }
            if (length < 0)
            { throw new ArgumentOutOfRangeException(nameof(length), "length must be non-negative"); }
            if (offset + length > buffer.Length)
            { throw new ArgumentOutOfRangeException("offset and length go out of bounds of the buffer"); }

            if (length < 19)
            { throw new ArgumentOutOfRangeException(nameof(length), "length of the payload must be at least 19 bytes"); }

            byte[] encodedName = Encoding.UTF8.GetBytes(name);
            if (encodedName.Length >= 15)
            { throw new ArgumentException($"UTF-8 encoding of {name} is {encodedName.Length} long, exceeding 14-byte maximum", nameof(name)); }

            byte preexisingNameChecksum = (byte)buffer.Skip(offset + 6).Take(15).Sum(x => x);
            byte newNameChecksum = (byte)encodedName.Sum(x => x);

            buffer[offset + 1] += (byte)(newNameChecksum - preexisingNameChecksum);
            Array.Copy(encodedName, 0, buffer, offset + 6, encodedName.Length);
            for (int i = encodedName.Length; i < 15; ++i)
            {
                buffer[offset + 6 + i] = 0;
            }
        }
    }
}
