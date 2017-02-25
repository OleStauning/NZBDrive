
namespace NZBDrive.WPFMessageView.Commands
{
    using System;
    using System.Windows;
    using System.Windows.Input;
    /// <summary>
    /// This class facilitates associating a key binding in XAML markup to a command
    /// defined in a View Model by exposing a Command dependency property.
    /// The class derives from Freezable to work around a limitation in WPF when data-binding from XAML.
    /// </summary>
    public class CommandReference : Freezable, ICommand
    {
        public event EventHandler CanExecuteChanged;

        public static readonly DependencyProperty CommandProperty = DependencyProperty.Register
        (
            "Command",
            typeof(ICommand),
            typeof(CommandReference),
            new PropertyMetadata(new PropertyChangedCallback((x, y) =>
            {
                var commandReference = x as CommandReference;
                var oldCommand = y.OldValue as ICommand;
                var newCommand = y.NewValue as ICommand;

                if (oldCommand != null) oldCommand.CanExecuteChanged -= commandReference.CanExecuteChanged;
                if (newCommand != null) newCommand.CanExecuteChanged += commandReference.CanExecuteChanged;
            }))
         );

        public ICommand Command
        {
            get { return (ICommand)GetValue(CommandProperty); }
            set { SetValue(CommandProperty, value); }
        }

        public bool CanExecute(object parameter)
        {
            if (Command != null) return Command.CanExecute(parameter);
            return false;
        }

        public void Execute(object parameter)
        {
            Command.Execute(parameter);
        }

        protected override Freezable CreateInstanceCore()
        {
            throw new NotImplementedException();
        }
    }
}
