using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;
using System.Windows.Markup;

namespace NZBDrive.View
{
    /// <summary>
    /// Markup extension that provides a list of the members of a given enum.
    /// </summary>
    public class EnumListExtension : MarkupExtension
    {
        #region Member Variables
 

        private Type _enumType;
        private bool _asString;
 

        #endregion //Member Variables
 

        #region Constructor
        /// <summary>
        /// Initializes a new <see cref=”EnumListExtension”/>
        /// </summary>
        public EnumListExtension()
        {
        }
 

        /// <summary>
        /// Initializes a new <see cref=”EnumListExtension”/>
        /// </summary>
        /// <param name=”enumType”>The type of enum whose members are to be returned.</param>
        public EnumListExtension(Type enumType)
        {
            this.EnumType = enumType;
        }
        #endregion //Constructor
 

        #region Properties
        /// <summary>
        /// Gets/sets the type of enumeration to return
        /// </summary>
        public Type EnumType
        {
            get { return this._enumType; }
            set
            {
                if (value != this._enumType)
                {
                    if (null != value)
                    {
                        Type enumType = Nullable.GetUnderlyingType(value) ?? value;
 

                        if (enumType.IsEnum == false)
                            throw new ArgumentException("Type must be for an Enum.");
                    }
 

                    this._enumType = value;
                }
            }
        }
 

        /// <summary>
        /// Gets/sets a value indicating whether to display the enumeration members as strings using the Description on the member if available.
        /// </summary>
        public bool AsString
        {
            get { return this._asString; }
            set { this._asString = value; }
        }
        #endregion //Properties
 

        #region Base class overrides
        /// <summary>
        /// Returns a list of items for the specified <see cref=”EnumType”/>. Depending on the <see cref=”AsString”/> property, the
        /// items will be returned as the enum member value or as strings.
        /// </summary>
        /// <param name=”serviceProvider”>An object that provides services for the markup extension.</param>
        /// <returns></returns>
        public override object ProvideValue(IServiceProvider serviceProvider)
        {
            if (null == this._enumType)
                throw new InvalidOperationException("The EnumType must be specified.");
 

            Type actualEnumType = Nullable.GetUnderlyingType(this._enumType) ?? this._enumType;
            Array enumValues = Enum.GetValues(actualEnumType);
 

            // if the object itself is to be returned then just use GetValues
            //
            if (this._asString == false)
            {
                if (actualEnumType == this._enumType)
                    return enumValues;
 

                Array tempArray = Array.CreateInstance(actualEnumType, enumValues.Length + 1);
                enumValues.CopyTo(tempArray, 1);
                return tempArray;
            }
 

            List<string> items = new List<string>();
 

            if (actualEnumType != this._enumType)
                items.Add(null);
 

            // otherwise we must process the list
            foreach (object item in Enum.GetValues(this._enumType))
            {
                string itemString = item.ToString();
                FieldInfo field = this._enumType.GetField(itemString);
                object[] attribs = field.GetCustomAttributes(typeof(DescriptionAttribute), false);
 

                if (null != attribs && attribs.Length > 0)
                    itemString = ((DescriptionAttribute)attribs[0]).Description;
 

                items.Add(itemString);
            }
 

            return items.ToArray();
        }
        #endregion //Base class overrides
    }

    public class EnumConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (value == null) return DependencyProperty.UnsetValue;

            return GetDescription((Enum)value);
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            return Enum.ToObject(targetType, value);
        }

        public static string GetDescription(Enum en)
        {
            Type type = en.GetType();
            MemberInfo[] memInfo = type.GetMember(en.ToString());
            if (memInfo != null && memInfo.Length > 0)
            {
                object[] attrs = memInfo[0].GetCustomAttributes(typeof(DescriptionAttribute), false);
                if (attrs != null && attrs.Length > 0)
                {
                    return ((DescriptionAttribute)attrs[0]).Description;
                }
            }
            return en.ToString();
        }
    }

}
