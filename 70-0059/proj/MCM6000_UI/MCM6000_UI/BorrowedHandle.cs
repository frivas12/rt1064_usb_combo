using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MCM6000_UI
{
    /// <summary>
    /// Visitor utility functions for borrowed handles.
    /// </summary>
    public static class BorrowedHandle
    {
        public static void With<TValue>(Func<BorrowedHandle<TValue>> factory, Action<TValue> visitor)
            where TValue : class
        {
            using var handle = factory();
            visitor(handle.Value);
        }

        public static TResult With<TValue, TResult>(Func<BorrowedHandle<TValue>> factory, Func<TValue, TResult> visitor)
            where TValue : class
        {
            using var handle = factory();
            return visitor(handle.Value);
        }
    }

    /// <summary>
    /// An abstract RAII handle to an object that invokes the derived
    /// class's dispose method on release.
    /// </summary>
    /// <typeparam name="T">The object being borrowed.</typeparam>
    public abstract class BorrowedHandle<T> : IDisposable
        where T : class
    {
        public T Value => _disposed ? throw new ObjectDisposedException(GetType().Name) : _value;

        public void Dispose()
        {
            GC.SuppressFinalize(this);
            DisposeHelper(true);
        }

        ~BorrowedHandle()
        {
            DisposeHelper(false);
        }


        protected BorrowedHandle(T value)
        {
            _value = value;
        }

        /// <summary>
        /// The logic to execute when dispose is called.
        /// Note:  This is guaranteed to only execute once.
        /// </summary>
        /// <param name="value">The value being disposed.</param>
        /// <param name="manual">If the dispose method was called manual rather than through the cleanup call.</param>
        protected abstract void Dispose(T value, bool manual);


        private readonly T _value;
        private bool _disposed = false;

        private void DisposeHelper(bool manual)
        {
            if (_disposed) return;

            Dispose(_value, manual);

            _disposed = true;
        }
    }

    public static class DeleterHandle
    {
        public static DeleterHandle<T> Create<T> (T value, Action<T, bool> deleter)
            where T : class => new(value, deleter);

        public static DeleterHandle<T> Create<T> (T value, Action<T> deleter)
            where T : class => new(value, deleter);

        public static DeleterHandle<T> Create<T> (T value, Action deleter)
            where T : class => new(value, deleter);
    }

    public class DeleterHandle<T> : BorrowedHandle<T>
        where T : class
    {
        /// <param name="value">The value to allow borrowing.</param>
        /// <param name="deleter">The callback to execute for disposal.  Provided the value and if the value was disposed manually (not by GC).</param>
        public DeleterHandle(T value, Action<T, bool> deleter)
            : base(value)
        {
            _deleter = deleter;
        }

        /// <param name="value">The value to allow borrowing.</param>
        /// <param name="deleter">The callback to execute for disposal.  Provided the value.</param>
        public DeleterHandle(T value, Action<T> deleter)
            : base(value)
        {
            _deleter = (x, b) =>
            {
                if (!b) { return; }
                deleter(x);
            };
        }

        /// <param name="value">The value to allow borrowing.</param>
        /// <param name="deleter">The callback to execute for disposal.</param>
        public DeleterHandle(T value, Action deleter)
            : base(value)
        {
            _deleter = (_, b) =>
            {
                if (!b) { return; }
                deleter();
            };
        }


        protected override void Dispose(T value,bool manual) => _deleter(value, manual);

        private readonly Action<T, bool> _deleter;
    }
}
