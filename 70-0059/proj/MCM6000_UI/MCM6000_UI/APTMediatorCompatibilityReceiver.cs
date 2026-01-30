using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using thorlabs.imaging_systems.apt_mediator.core;

namespace MCM6000_UI
{
    /// <summary>
    /// A class that constantly accepts any and all APT command dispatched to it.
    /// </summary>
    public class APTMediatorCompatibilityReceiver : IDisposable
    {
        public event EventHandler<APTCommand> CommandReceived;

        public void Dispose()
        {
            Dispose(disposing: true);
            GC.SuppressFinalize(this);
        }


        public APTMediatorCompatibilityReceiver(IAPTMediator applicationMediator, IAPTMediator bootloaderMediator)
        {
            void OnCommandReceived(object sender, EventArgs _)
            {
                IAPTReceiverQueue queue = (IAPTReceiverQueue)sender;
                CommandReceived.Invoke(this, queue.TryDequeue());
            }
            void AddReceiver(IAPTReceiverQueue queue)
            {
                queue.CommandReceived += OnCommandReceived;
            }

            _receiverMotherboardStandalone = bootloaderMediator.Listen(_ => true, APTAddress.From(APTAddress.StandardEnumerations.MOTHERBOARD_STANDALONE));;
            _receiverMotherboard = applicationMediator.Listen(_ => true, APTAddress.From(APTAddress.StandardEnumerations.MOTHERBOARD));;
            _receiverSlot1 = applicationMediator.Listen(_ => true, APTAddress.From(APTAddress.StandardEnumerations.SLOT_1));
            _receiverSlot2 = applicationMediator.Listen(_ => true, APTAddress.From(APTAddress.StandardEnumerations.SLOT_2));
            _receiverSlot3 = applicationMediator.Listen(_ => true, APTAddress.From(APTAddress.StandardEnumerations.SLOT_3));
            _receiverSlot4 = applicationMediator.Listen(_ => true, APTAddress.From(APTAddress.StandardEnumerations.SLOT_4));
            _receiverSlot5 = applicationMediator.Listen(_ => true, APTAddress.From(APTAddress.StandardEnumerations.SLOT_5));
            _receiverSlot6 = applicationMediator.Listen(_ => true, APTAddress.From(APTAddress.StandardEnumerations.SLOT_6));
            _receiverSlot7 = applicationMediator.Listen(_ => true, APTAddress.From(APTAddress.StandardEnumerations.SLOT_7));

            AddReceiver(_receiverMotherboard); 
            AddReceiver(_receiverMotherboardStandalone); 
            AddReceiver(_receiverSlot1); 
            AddReceiver(_receiverSlot2); 
            AddReceiver(_receiverSlot3); 
            AddReceiver(_receiverSlot4); 
            AddReceiver(_receiverSlot5); 
            AddReceiver(_receiverSlot6); 
            AddReceiver(_receiverSlot7); 
        }

        private readonly IAPTReceiverQueue _receiverMotherboard;
        private readonly IAPTReceiverQueue _receiverMotherboardStandalone;
        private readonly IAPTReceiverQueue _receiverSlot1;
        private readonly IAPTReceiverQueue _receiverSlot2;
        private readonly IAPTReceiverQueue _receiverSlot3;
        private readonly IAPTReceiverQueue _receiverSlot4;
        private readonly IAPTReceiverQueue _receiverSlot5;
        private readonly IAPTReceiverQueue _receiverSlot6;
        private readonly IAPTReceiverQueue _receiverSlot7;


        protected virtual void Dispose(bool disposing)
        {
            if (!_disposed)
            {
                if (disposing)
                {
                    _receiverMotherboard.Dispose();
                    _receiverMotherboardStandalone.Dispose();
                    _receiverSlot1.Dispose();
                    _receiverSlot2.Dispose();
                    _receiverSlot3.Dispose();
                    _receiverSlot4.Dispose();
                    _receiverSlot5.Dispose();
                    _receiverSlot6.Dispose();
                    _receiverSlot7.Dispose();
                }

                _disposed = true;
            }
        }


        private bool _disposed;
    }
}
