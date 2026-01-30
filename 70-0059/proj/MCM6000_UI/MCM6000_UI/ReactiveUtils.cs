using ReactiveUI;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Linq.Expressions;
using System.Reactive;
using System.Reactive.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace MCM6000_UI
{
    public static class ReactiveUtils
    {
        public static ICommand BindCommand<TParam>(ReactiveCommand<TParam, Unit> command, TParam value) =>
            ReactiveCommand.CreateFromObservable(() => command.Execute(value), command.CanExecute);

        public static ReactiveCommand<Unit, TResult> BindCommand<TParam, TResult>(ReactiveCommand<TParam, TResult> command, TParam value) =>
            ReactiveCommand.CreateFromObservable(() => command.Execute(value), command.CanExecute);
    }
}
