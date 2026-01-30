using MCM6000_UI.controls.drivers;
using ReactiveUI;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace MCM6000_UI.controls.vms
{
    public class SavedPositionsEditorVM : ReactiveObject
    {
        public IStepperDriver.ISavedPosition SelectedPosition
        {
            get => _selected;
            set => this.RaiseAndSetIfChanged(ref _selected, value);
        }

        public string SelectedPositionScratchpad
        {
            get => _scratchpad;
            set => this.RaiseAndSetIfChanged(ref _scratchpad, value);
        }

        public IEnumerable<IStepperDriver.ISavedPosition> Positions => _positions.Value;

        
        public ICommand ClearSelectedPosition
        { get; }

        public ICommand SetSelectedPosition
        { get; }

        public ICommand CopyCurrentPositionToSelectedPosition
        { get; }

        public ICommand SaveAllChanges
        { get; }

        public ICommand RevertAllChanges
        { get; }


        public IStepperDriver Driver
        {
            get => _driver;
            set => this.RaiseAndSetIfChanged(ref _driver, value);
        }


        public SavedPositionsEditorVM()
        {
            _positions = this
                .WhenAnyValue(x => x.Driver)
                .Select(x => x is not null ? x.WhenAnyValue(y => y.SavedPositions) : Observable.Return(Enumerable.Empty<IStepperDriver.ISavedPosition>()))
                .Switch()
                .ToProperty(this, x => x.Positions);

            var canExecuteCommand =
                Observable.CombineLatest(
                    this.WhenAnyValue(x => x.Driver).Select(x => x is not null),
                    this.WhenAnyValue(x => x.SelectedPosition).Select(x => x is not null),
                    (l, r) => l && r
                );
            var canTranslateScratchpad = this
                .WhenAnyValue(x => x.SelectedPositionScratchpad)
                .Throttle(TimeSpan.FromMilliseconds(400))
                .Select(x => int.TryParse(x, out _));

            ClearSelectedPosition = ReactiveCommand.Create(() =>
            {
                SelectedPosition.ClearPosition();
            }, canExecuteCommand);

            SetSelectedPosition = ReactiveCommand.Create(() =>
            {
                SelectedPosition.Position = int.Parse(SelectedPositionScratchpad);
            }, canExecuteCommand.CombineLatest(canTranslateScratchpad, (l, r) => l && r));

            CopyCurrentPositionToSelectedPosition = ReactiveCommand.Create(() =>
            {
                SelectedPosition.Position = Driver.EncoderPosition;
            }, canExecuteCommand);

            SaveAllChanges = ReactiveCommand.CreateFromTask(async token =>
            {
                await Driver.CommitSavedPositionsAsync(token);
            }, canExecuteCommand);

            RevertAllChanges = ReactiveCommand.CreateFromTask(async token =>
            {
                await Driver.RestoreSavedPositionsAsync(token);
            }, canExecuteCommand);
        }

        private readonly ObservableAsPropertyHelper<IEnumerable<IStepperDriver.ISavedPosition>> _positions;

        private IStepperDriver.ISavedPosition _selected = null;
        private string _scratchpad = string.Empty;

        private IStepperDriver _driver = null;
    }
}
