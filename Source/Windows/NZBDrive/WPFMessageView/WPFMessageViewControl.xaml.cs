using NZBDrive.WPFMessageView.Commands;
using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace NZBDrive.WPFMessageView
{
    /// <summary>
    /// Interaction logic for WPFMessageView.xaml
    /// </summary>
    public class WPFMessageViewControl : UserControl
    {
        public WPFMessageViewControl()
        {
            this.DefaultStyleKey = typeof(WPFMessageViewControl);

            YesCommand = new DelegateCommand(() =>
            {
                Result = WPFMessageViewResult.Yes;
                Close();
            });

            NoCommand = new DelegateCommand(() =>
            {
                Result = WPFMessageViewResult.No;
                Close();
            });

            CancelCommand = new DelegateCommand(() =>
            {
                Result = WPFMessageViewResult.Cancel;
                Close();
            });

            CancelCommand = new DelegateCommand(() =>
            {
                Result = WPFMessageViewResult.Close;
                Close();
            });

            OkCommand = new DelegateCommand(() =>
            {
                Result = WPFMessageViewResult.Ok;
                Close();
            });
        }

        private Button ButtonYes { get { return GetTemplateChild("ButtonYes") as Button; } }
        private Button ButtonNo { get { return GetTemplateChild("ButtonNo") as Button; } }
        private Button ButtonCancel { get { return GetTemplateChild("ButtonCancel") as Button; } }
        private Button ButtonOK { get { return GetTemplateChild("ButtonOK") as Button; } }
        private Button ButtonClose { get { return GetTemplateChild("ButtonClose") as Button; } }

        public override void OnApplyTemplate()
        {
            ButtonYes.IsEnabled = ButtonYesIsEnabled;
            ButtonNo.IsEnabled = ButtonNoIsEnabled;
            ButtonCancel.IsEnabled = ButtonCancelIsEnabled;
            ButtonOK.IsEnabled = ButtonOKIsEnabled;
            ButtonClose.IsEnabled = ButtonCloseIsEnabled;

            SetButtonVisibility(ButtonVisibility);
        }

        public WPFMessageViewResult Result { get; set; }
        public void Close() 
        {
            var dialog = Window.GetWindow(this);
            dialog.DialogResult = true;
            dialog.Close(); 
        }

        public object DetailsContent
        {
            get { return GetValue(DetailsContentProperty); }
            set { SetValue(DetailsContentProperty, value); }
        }

        // Using a DependencyProperty as the backing store for MyProperty.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty DetailsContentProperty =
            DependencyProperty.Register("DetailsContent", typeof(object), typeof(WPFMessageViewControl), null);

        public ImageSource ImageSource
        {
            get { return GetValue(ImageSourceProperty) as ImageSource; }
            set { SetValue(ImageSourceProperty, value); }
        }

        public static readonly DependencyProperty ImageSourceProperty =
            DependencyProperty.Register("ImageSource", typeof(ImageSource), typeof(WPFMessageViewControl), null);

        public Visibility ShowDetails
        {
            get { return (Visibility)GetValue(ShowDetailsProperty); }
            set { SetValue(ShowDetailsProperty, value); }
        }

        public static readonly DependencyProperty ShowDetailsProperty =
            DependencyProperty.Register("ShowDetails", typeof(Visibility), typeof(WPFMessageViewControl), new PropertyMetadata(Visibility.Collapsed));


        private static readonly DependencyPropertyKey YesNoVisibilityPropertyKey
            = DependencyProperty.RegisterReadOnly("YesNoVisibility", typeof(Visibility), typeof(WPFMessageViewControl), null);

        public static readonly DependencyProperty YesNoVisibilityProperty = YesNoVisibilityPropertyKey.DependencyProperty;

        public Visibility YesNoVisibility
        {
            get { return (Visibility)GetValue(YesNoVisibilityProperty); }
            protected set { SetValue(YesNoVisibilityPropertyKey, value); }
        }

        private static readonly DependencyPropertyKey CancelVisibilityPropertyKey
            = DependencyProperty.RegisterReadOnly("CancelVisibility", typeof(Visibility), typeof(WPFMessageViewControl), null);

        public static readonly DependencyProperty CancelVisibilityProperty = CancelVisibilityPropertyKey.DependencyProperty;

        public Visibility CancelVisibility
        {
            get { return (Visibility)GetValue(CancelVisibilityProperty); }
            protected set { SetValue(CancelVisibilityPropertyKey, value); }
        }

        private static readonly DependencyPropertyKey OkVisibilityPropertyKey
            = DependencyProperty.RegisterReadOnly("OkVisibility", typeof(Visibility), typeof(WPFMessageViewControl), null);

        public static readonly DependencyProperty OkVisibilityProperty = OkVisibilityPropertyKey.DependencyProperty;

        public Visibility OkVisibility
        {
            get { return (Visibility)GetValue(OkVisibilityProperty); }
            protected set { SetValue(OkVisibilityPropertyKey, value); }
        }

        private static readonly DependencyPropertyKey CloseVisibilityPropertyKey
            = DependencyProperty.RegisterReadOnly("CloseVisibility", typeof(Visibility), typeof(WPFMessageViewControl), null);

        public static readonly DependencyProperty CloseVisibilityProperty = CloseVisibilityPropertyKey.DependencyProperty;

        public Visibility CloseVisibility
        {
            get { return (Visibility)GetValue(CloseVisibilityProperty); }
            protected set { SetValue(CloseVisibilityPropertyKey, value); }
        }

        public void SetButtonVisibility(WPFMessageViewButtons visibility)
        {
            SetValue(YesNoVisibilityPropertyKey, Visibility.Collapsed);
            SetValue(CancelVisibilityPropertyKey, Visibility.Collapsed);
            SetValue(OkVisibilityPropertyKey, Visibility.Collapsed);
            SetValue(CloseVisibilityPropertyKey, Visibility.Collapsed);
            switch (visibility)
            {
                case WPFMessageViewButtons.YesNo:
                    SetValue(YesNoVisibilityPropertyKey, Visibility.Visible);
                    if (ButtonYes!=null) ButtonYes.IsDefault = true;
                    if (ButtonNo != null) ButtonNo.IsCancel = true;
                    break;
                case WPFMessageViewButtons.YesNoCancel:
                    SetValue(YesNoVisibilityPropertyKey, Visibility.Visible);
                    SetValue(CancelVisibilityPropertyKey, Visibility.Visible);
                    if (ButtonYes != null) ButtonYes.IsDefault = true;
                    if (ButtonCancel != null) ButtonCancel.IsCancel = true;
                    break;
                case WPFMessageViewButtons.OK:
                    SetValue(OkVisibilityPropertyKey, Visibility.Visible);
                    if (ButtonOK != null) ButtonOK.IsDefault = true;
                    if (ButtonOK != null) ButtonOK.IsCancel = true;
                    break;
                case WPFMessageViewButtons.OKClose:
                    SetValue(OkVisibilityPropertyKey, Visibility.Visible);
                    SetValue(CloseVisibilityPropertyKey, Visibility.Visible);
                    if (ButtonOK != null) ButtonOK.IsDefault = true;
                    if (ButtonClose != null) ButtonClose.IsCancel = true;
                    break;
                case WPFMessageViewButtons.OKCancel:
                    SetValue(OkVisibilityPropertyKey, Visibility.Visible);
                    SetValue(CancelVisibilityPropertyKey, Visibility.Visible);
                    if (ButtonOK != null) ButtonOK.IsDefault = true;
                    if (ButtonCancel != null) ButtonCancel.IsCancel = true;
                    break;
                default:
                    SetValue(CloseVisibilityPropertyKey, Visibility.Visible);
                    if (ButtonClose != null) ButtonClose.IsDefault = true;
                    if (ButtonClose != null) ButtonClose.IsCancel = true;
                    break;

            }
        
        }

        public static readonly DependencyProperty ButtonVisibilityProperty =
            DependencyProperty.Register("ButtonVisibility", typeof(WPFMessageViewButtons), typeof(WPFMessageViewControl), 
            new PropertyMetadata(WPFMessageViewButtons.OK, new PropertyChangedCallback(OnButtonVisibilityChanged), null ));

        private static void OnButtonVisibilityChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var me = d as WPFMessageViewControl;
            me.SetButtonVisibility((WPFMessageViewButtons)e.NewValue);
        }

        public WPFMessageViewButtons ButtonVisibility
        {
            get { return (WPFMessageViewButtons)GetValue(ButtonVisibilityProperty); }
            set { SetValue(ButtonVisibilityProperty, value); }
        }

        public static readonly DependencyProperty ButtonYesIsEnabledProperty
            = DependencyProperty.Register("ButtonYesIsEnabled", typeof(bool), typeof(WPFMessageViewControl),
            new PropertyMetadata(true, new PropertyChangedCallback((c, a) =>
            {
                var control = c as WPFMessageViewControl;
                if (control.ButtonYes != null) control.ButtonYes.IsEnabled = (bool)a.NewValue;
            })));

        public bool ButtonYesIsEnabled
        {
            get { return (bool)GetValue(ButtonYesIsEnabledProperty); }
            set { SetValue(ButtonYesIsEnabledProperty, value); }
        }

        public static readonly DependencyProperty ButtonNoIsEnabledProperty
            = DependencyProperty.Register("ButtonNoIsEnabled", typeof(bool), typeof(WPFMessageViewControl),
            new PropertyMetadata(true, new PropertyChangedCallback((c, a) =>
            {
                var control = c as WPFMessageViewControl;
                if (control.ButtonNo != null) control.ButtonNo.IsEnabled = (bool)a.NewValue;
            })));

        public bool ButtonNoIsEnabled
        {
            get { return (bool)GetValue(ButtonNoIsEnabledProperty); }
            set { SetValue(ButtonNoIsEnabledProperty, value); }
        }

        public static readonly DependencyProperty ButtonCancelIsEnabledProperty
            = DependencyProperty.Register("ButtonCancelIsEnabled", typeof(bool), typeof(WPFMessageViewControl),
            new PropertyMetadata(true, new PropertyChangedCallback((c, a) =>
            {
                var control = c as WPFMessageViewControl;
                if (control.ButtonCancel != null) control.ButtonCancel.IsEnabled = (bool)a.NewValue;
            })));

        public bool ButtonCancelIsEnabled
        {
            get { return (bool)GetValue(ButtonCancelIsEnabledProperty); }
            set { SetValue(ButtonCancelIsEnabledProperty, value); }
        }

        public static readonly DependencyProperty ButtonCloseIsEnabledProperty
            = DependencyProperty.Register("ButtonCloseIsEnabled", typeof(bool), typeof(WPFMessageViewControl),
            new PropertyMetadata(true, new PropertyChangedCallback((c, a) =>
            {
                var control = c as WPFMessageViewControl;
                if (control.ButtonClose != null) control.ButtonClose.IsEnabled = (bool)a.NewValue;
            })));

        public bool ButtonCloseIsEnabled
        {
            get { return (bool)GetValue(ButtonCloseIsEnabledProperty); }
            set { SetValue(ButtonCloseIsEnabledProperty, value); }
        }

        public static readonly DependencyProperty ButtonOKIsEnabledProperty
            = DependencyProperty.Register("ButtonOKIsEnabled", typeof(bool), typeof(WPFMessageViewControl),
            new PropertyMetadata(true, new PropertyChangedCallback((c, a) =>
            { 
                var control = c as WPFMessageViewControl;
                if (control.ButtonOK != null) control.ButtonOK.IsEnabled = (bool)a.NewValue;
            })));

        public bool ButtonOKIsEnabled
        {
            get { return (bool)GetValue(ButtonOKIsEnabledProperty); }
            set { SetValue(ButtonOKIsEnabledProperty, value); }
        }


        private static readonly DependencyPropertyKey YesCommandPropertyKey
            = DependencyProperty.RegisterReadOnly("YesCommand", typeof(ICommand), typeof(WPFMessageViewControl), null);

        public static readonly DependencyProperty YesCommandProperty = YesCommandPropertyKey.DependencyProperty;

        public ICommand YesCommand
        {
            get { return (ICommand)GetValue(YesCommandProperty); }
            protected set { SetValue(YesCommandPropertyKey, value); }
        }

        private static readonly DependencyPropertyKey NoCommandPropertyKey
            = DependencyProperty.RegisterReadOnly("NoCommand", typeof(ICommand), typeof(WPFMessageViewControl), null);

        public static readonly DependencyProperty NoCommandProperty = NoCommandPropertyKey.DependencyProperty;

        public ICommand NoCommand
        {
            get { return (ICommand)GetValue(NoCommandProperty); }
            protected set { SetValue(NoCommandPropertyKey, value); }
        }

        private static readonly DependencyPropertyKey CancelCommandPropertyKey
            = DependencyProperty.RegisterReadOnly("CancelCommand", typeof(ICommand), typeof(WPFMessageViewControl), null);

        public static readonly DependencyProperty CancelCommandProperty = CancelCommandPropertyKey.DependencyProperty;

        public ICommand CancelCommand
        {
            get { return (ICommand)GetValue(CancelCommandProperty); }
            protected set { SetValue(CancelCommandPropertyKey, value); }
        }

        private static readonly DependencyPropertyKey CloseCommandPropertyKey
            = DependencyProperty.RegisterReadOnly("CloseCommand", typeof(ICommand), typeof(WPFMessageViewControl), null);

        public static readonly DependencyProperty CloseCommandProperty = CloseCommandPropertyKey.DependencyProperty;

        public ICommand CloseCommand
        {
            get { return (ICommand)GetValue(CloseCommandProperty); }
            protected set { SetValue(CloseCommandPropertyKey, value); }
        }

        private static readonly DependencyPropertyKey OkCommandPropertyKey
                    = DependencyProperty.RegisterReadOnly("OkCommand", typeof(ICommand), typeof(WPFMessageViewControl), null);

        public static readonly DependencyProperty OkCommandProperty = OkCommandPropertyKey.DependencyProperty;

        public ICommand OkCommand
        {
            get { return (ICommand)GetValue(OkCommandProperty); }
            protected set { SetValue(OkCommandPropertyKey, value); }
        }

        /*
                ICommand ___YesCommand;
                ICommand ___NoCommand;
                ICommand ___CancelCommand;
                ICommand ___CloseCommand;
                ICommand ___OKCommand;
         * /
                /*
                public ICommand YesCommand
                {
                    get
                    {
                        if (___YesCommand == null)
                            ___YesCommand = new DelegateCommand(() =>
                            {
                                Result = WPFMessageViewResult.Yes;
                                Close();
                            });
                        return ___YesCommand;
                    }
                }
                public ICommand NoCommand
                {
                    get
                    {
                        if (___NoCommand == null)
                            ___NoCommand = new DelegateCommand(() =>
                            {
                                Result = WPFMessageViewResult.No;
                                Close();
                            });
                        return ___NoCommand;
                    }
                }
                

        public ICommand CancelCommand
        {
            get
            {
                if (___CancelCommand == null)
                    ___CancelCommand = new DelegateCommand(() =>
                    {
                        Result = WPFMessageViewResult.Cancel;
                        Close();
                    });
                return ___CancelCommand;
            }
        }


        public ICommand CloseCommand
        {
            get
            {
                if (___CloseCommand == null)
                    ___CloseCommand = new DelegateCommand(() =>
                    {
                        Result = WPFMessageViewResult.Close;
                        Close();
                    });
                return ___CloseCommand;
            }
        }

        public ICommand OkCommand
        {
            get
            {
                if (___OKCommand == null)
                    ___OKCommand = new DelegateCommand(() =>
                    {
                        Result = WPFMessageViewResult.Ok;
                        Close();
                    });
                return ___OKCommand;
            }
        }
        */

    }
}
