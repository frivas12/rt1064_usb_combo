using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MCM6000_UI
{
    /// <summary>
    /// Binds the lifetime of a disposable resource that can generate a stream
    /// to the lifetime of its generated stream.
    /// </summary>
    internal sealed class DependentStream : Stream
    {
        /// <summary>
        /// Creates a new proxy from the resource.
        /// By default, any exception occuring during object creation will dispose the resource.
        /// </summary>
        public static DependentStream Open<T>(T resource, Func<T, Stream> streamFromResources, bool disposeOnException = true)
            where T : IDisposable
        {
            try
            {
                var stream = streamFromResources(resource);
                return stream is null
                    ? throw new NullReferenceException("the returned stream was null")
                    : new DependentStream(resource, stream);
            }
            catch
            {
                if (disposeOnException)
                {
                    resource.Dispose();
                }
                throw;
            }
        }

        public override bool CanRead => _stream.CanRead;
        public override bool CanSeek => _stream.CanSeek;
        public override bool CanWrite => _stream.CanWrite;
        public override long Length => _stream.Length;

        public override long Position
        {
            get => _stream.Position;
            set => _stream.Position = value;
        }

        public override void Flush() => _stream.Flush();
        public override int Read(byte[] buffer, int offset, int count) => _stream.Read(buffer, offset,  count);
        public override long Seek(long offset, SeekOrigin origin) => _stream.Seek(offset, origin);
        public override void SetLength(long value) => _stream.SetLength(value);
        public override void Write(byte[] buffer, int offset, int count) => _stream.Write(buffer, offset, count);

        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);

            _stream.Dispose();
            _ownedDependency.Dispose();
        }

        private readonly IDisposable _ownedDependency;
        private readonly Stream _stream;

        private DependentStream(IDisposable resource, Stream stream)
        {
            _ownedDependency = resource;
            _stream = stream;
        }
    }
}
