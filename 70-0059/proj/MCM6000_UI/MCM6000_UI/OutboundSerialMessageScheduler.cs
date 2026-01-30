using Microsoft.Office.Interop.Excel;
using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace MCM6000_UI
{
    /// <summary>
    /// A manager that uses a priority FIFO structure manage outbound messages over a serial port.
    /// </summary>
    public class OutboundSerialMessageScheduler
    {
        public const int IMMEDIATE_PRIORITY = int.MaxValue;

        public SerialPort Port
        { get; }

        public int CooldownMs
        { get; set; } = 15;


        /// <summary>
        /// Either sends the message immediately or schedules the message to be sent,
        /// depending on priority.
        /// Thread-safe.
        /// </summary>
        public void Send(byte[] data, int offset, int length, int priority)
        {
            if (priority == IMMEDIATE_PRIORITY)
            {
                SendAsap(data, offset, length);
            } else
            {
                SendScheduled(data, offset, length, priority);
            }
        }

        public void SendAsap(byte[] data, int offset, int length)
        {
            lock (_lock)
            {
                WaitForCooldown();
                Port.Write(data, offset, length);
                _cooldownFinished = DateTime.Now + TimeSpan.FromMilliseconds(CooldownMs);
            }
        }

        public void SendScheduled(byte[] data, int offset, int length, int priority = 0)
        {
            lock (_lock)
            {
                byte[] clone = data.Skip(offset).Take(length).ToArray();
                if (clone.Length == 0)
                    throw new ArgumentException("offset and length gives 0 bytes to send.");
                GetPriorityQueue(priority).Enqueue(new() { message = clone, offset = 0, length = clone.Length });
            }
            _elementCount.Release();
        }

        /// <summary>
        /// Unlike SendScheduled, the data is not cloned into the scheduler.
        /// </summary>
        public void SendScheduledNoCopy(byte[] data, int offset, int length, int priority = 0)
        {
            lock (_lock)
            {
                if (data.Length < offset + length)
                    throw new ArgumentException("offset and length gives 0 bytes to send.");
                GetPriorityQueue(priority).Enqueue(new() { message = data, offset = offset, length = length });
            }
            _elementCount.Release();
        }

        public bool HasScheduledSends()
        {
            return _elementCount.CurrentCount > 0;
        }

        public bool BlockAndSendScheduledMessage(TimeSpan timeout)
        {
            if (!_elementCount.Wait(timeout))
            { return false; }
            WaitForCooldown();
            SendNextMessage();

            return true;
        }

        public bool BlockAndSendScheduledMessage(int timeoutMs)
        {
            DateTime start = DateTime.Now;
            var rt = _elementCount.Wait(timeoutMs);
            timeoutMs -= (int)(DateTime.Now - start).TotalMilliseconds;
            if (rt)
            {
                rt = WaitForCooldown(timeoutMs);
            }
            if (rt)
            {
                SendNextMessage();
            }
            return rt;
        }

        public void BlockAndSendScheduledMessage(CancellationToken token)
        {
            _elementCount.Wait(token);
            token.ThrowIfCancellationRequested();
            WaitForCooldown();
            SendNextMessage();
        }

        public bool TrySendScheduledMessage()
        {
            return BlockAndSendScheduledMessage(0);
        }

        public async Task SendScheduledMessageAsync(CancellationToken token)
        {
            await _elementCount.WaitAsync(token);
            token.ThrowIfCancellationRequested();
            await AwaitCooldown();
            SendNextMessage();
        }

        public void DropScheduleMessages()
        {
            // Erases element count
            while (_elementCount.Wait(0)) { };
            lock (_lock)
            {
                foreach (var kvp in _queues)
                {
                    kvp.Value.Clear();
                }
                CleanupQueues();
            }
        }

        public void CleanupQueues()
        {
            lock (_lock)
            {
                foreach (var kvp in _queues)
                {
                    if (kvp.Key != 0 && !kvp.Value.Any())
                    {
                        _queues.Remove(kvp.Key);
                        _priorities.Remove(kvp.Key);
                    }
                }
            }
        }

        public BorrowedHandle<SerialPort> BorrowSerialPort()
        {
            Monitor.Enter(_lock);
            WaitForCooldown();
            return DeleterHandle.Create(Port, (_, _) =>
            {
                Monitor.Exit(_lock);
            });
        }

        public OutboundSerialMessageScheduler(SerialPort port)
        {
            Port = port;

            // Make 0 always available.
            _priorities.Add(0);
            _queues[0] = new();
        }

        private struct Message
        {
            public byte[] message;
            public int offset;
            public int length;
        }

        private readonly object _lock = new();
        private readonly SemaphoreSlim _elementCount = new(0);
        private readonly SortedSet<int> _priorities = [];
        private readonly Dictionary<int, Queue<Message>> _queues = [];

        private DateTime _cooldownFinished = DateTime.Now;

        private Queue<Message> GetPriorityQueue(int priority)
        {
            if (_priorities.Add(priority))
            {
                _queues[priority] = new();
            }

            return _queues[priority];
        }

        private Message GetNextMessage()
        {
            foreach (int priority in _priorities.Reverse())
            {
                if (_queues[priority].Any())
                {
                    return _queues[priority].Dequeue();
                }
            }

            throw new InvalidOperationException("GetNextMessage called without a message to return");
        }

        private void WaitForCooldown()
        {
            TimeSpan diff = _cooldownFinished - DateTime.Now;

            if (diff > TimeSpan.Zero)
            {
                Thread.Sleep(diff);
            }
        }

        private bool WaitForCooldown(int timeoutMs)
        {
            TimeSpan diff = _cooldownFinished - DateTime.Now;

            if (diff.TotalMilliseconds > timeoutMs)
            {
                return false;
            }

            if (diff > TimeSpan.Zero)
            {
                Thread.Sleep(diff);
            }

            return true;
        }

        private async Task AwaitCooldown()
        {
            TimeSpan diff = _cooldownFinished - DateTime.Now;

            if (diff > TimeSpan.Zero)
            {
                await Task.Delay(diff);
            }
        }

        private void SendNextMessage()
        {
            lock (_lock)
            {
                var message = GetNextMessage();
                Port.Write(message.message, message.offset, message.length);
                _cooldownFinished = DateTime.Now + TimeSpan.FromMilliseconds(CooldownMs);
            }
        }
    }
}
