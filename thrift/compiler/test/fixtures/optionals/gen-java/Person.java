/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;
import java.util.Set;
import java.util.HashSet;
import java.util.Collections;
import java.util.BitSet;
import java.util.Arrays;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.facebook.thrift.*;
import com.facebook.thrift.async.*;
import com.facebook.thrift.meta_data.*;
import com.facebook.thrift.server.*;
import com.facebook.thrift.transport.*;
import com.facebook.thrift.protocol.*;

@SuppressWarnings({ "unused", "serial" })
public class Person implements TBase, java.io.Serializable, Cloneable, Comparable<Person> {
  private static final TStruct STRUCT_DESC = new TStruct("Person");
  private static final TField ID_FIELD_DESC = new TField("id", TType.I64, (short)1);
  private static final TField NAME_FIELD_DESC = new TField("name", TType.STRING, (short)2);
  private static final TField AGE_FIELD_DESC = new TField("age", TType.I16, (short)3);
  private static final TField ADDRESS_FIELD_DESC = new TField("address", TType.STRING, (short)4);
  private static final TField FAVORITE_COLOR_FIELD_DESC = new TField("favoriteColor", TType.STRUCT, (short)5);
  private static final TField FRIENDS_FIELD_DESC = new TField("friends", TType.SET, (short)6);
  private static final TField BEST_FRIEND_FIELD_DESC = new TField("bestFriend", TType.I64, (short)7);
  private static final TField PET_NAMES_FIELD_DESC = new TField("petNames", TType.MAP, (short)8);
  private static final TField AFRAID_OF_ANIMAL_FIELD_DESC = new TField("afraidOfAnimal", TType.I32, (short)9);
  private static final TField VEHICLES_FIELD_DESC = new TField("vehicles", TType.LIST, (short)10);

  public long id;
  public String name;
  public short age;
  public String address;
  public Color favoriteColor;
  public Set<Long> friends;
  public long bestFriend;
  public Map<Animal,String> petNames;
  /**
   * 
   * @see Animal
   */
  public Animal afraidOfAnimal;
  public List<Vehicle> vehicles;
  public static final int ID = 1;
  public static final int NAME = 2;
  public static final int AGE = 3;
  public static final int ADDRESS = 4;
  public static final int FAVORITECOLOR = 5;
  public static final int FRIENDS = 6;
  public static final int BESTFRIEND = 7;
  public static final int PETNAMES = 8;
  public static final int AFRAIDOFANIMAL = 9;
  public static final int VEHICLES = 10;
  public static boolean DEFAULT_PRETTY_PRINT = true;

  // isset id assignments
  private static final int __ID_ISSET_ID = 0;
  private static final int __AGE_ISSET_ID = 1;
  private static final int __BESTFRIEND_ISSET_ID = 2;
  private static final int __AFRAIDOFANIMAL_ISSET_ID = 3;
  private BitSet __isset_bit_vector = new BitSet(4);

  public static final Map<Integer, FieldMetaData> metaDataMap;
  static {
    Map<Integer, FieldMetaData> tmpMetaDataMap = new HashMap<Integer, FieldMetaData>();
    tmpMetaDataMap.put(ID, new FieldMetaData("id", TFieldRequirementType.DEFAULT, 
        new FieldValueMetaData(TType.I64)));
    tmpMetaDataMap.put(NAME, new FieldMetaData("name", TFieldRequirementType.DEFAULT, 
        new FieldValueMetaData(TType.STRING)));
    tmpMetaDataMap.put(AGE, new FieldMetaData("age", TFieldRequirementType.OPTIONAL, 
        new FieldValueMetaData(TType.I16)));
    tmpMetaDataMap.put(ADDRESS, new FieldMetaData("address", TFieldRequirementType.OPTIONAL, 
        new FieldValueMetaData(TType.STRING)));
    tmpMetaDataMap.put(FAVORITECOLOR, new FieldMetaData("favoriteColor", TFieldRequirementType.OPTIONAL, 
        new StructMetaData(TType.STRUCT, Color.class)));
    tmpMetaDataMap.put(FRIENDS, new FieldMetaData("friends", TFieldRequirementType.OPTIONAL, 
        new SetMetaData(TType.SET, 
            new FieldValueMetaData(TType.I64))));
    tmpMetaDataMap.put(BESTFRIEND, new FieldMetaData("bestFriend", TFieldRequirementType.OPTIONAL, 
        new FieldValueMetaData(TType.I64)));
    tmpMetaDataMap.put(PETNAMES, new FieldMetaData("petNames", TFieldRequirementType.OPTIONAL, 
        new MapMetaData(TType.MAP, 
            new FieldValueMetaData(TType.I32), 
            new FieldValueMetaData(TType.STRING))));
    tmpMetaDataMap.put(AFRAIDOFANIMAL, new FieldMetaData("afraidOfAnimal", TFieldRequirementType.OPTIONAL, 
        new FieldValueMetaData(TType.I32)));
    tmpMetaDataMap.put(VEHICLES, new FieldMetaData("vehicles", TFieldRequirementType.OPTIONAL, 
        new ListMetaData(TType.LIST, 
            new StructMetaData(TType.STRUCT, Vehicle.class))));
    metaDataMap = Collections.unmodifiableMap(tmpMetaDataMap);
  }

  static {
    FieldMetaData.addStructMetaDataMap(Person.class, metaDataMap);
  }

  public Person() {
  }

  public Person(
    long id,
    String name)
  {
    this();
    this.id = id;
    setIdIsSet(true);
    this.name = name;
  }

  public Person(
    long id,
    String name,
    short age,
    String address,
    Color favoriteColor,
    Set<Long> friends,
    long bestFriend,
    Map<Animal,String> petNames,
    Animal afraidOfAnimal,
    List<Vehicle> vehicles)
  {
    this();
    this.id = id;
    setIdIsSet(true);
    this.name = name;
    this.age = age;
    setAgeIsSet(true);
    this.address = address;
    this.favoriteColor = favoriteColor;
    this.friends = friends;
    this.bestFriend = bestFriend;
    setBestFriendIsSet(true);
    this.petNames = petNames;
    this.afraidOfAnimal = afraidOfAnimal;
    setAfraidOfAnimalIsSet(true);
    this.vehicles = vehicles;
  }

  /**
   * Performs a deep copy on <i>other</i>.
   */
  public Person(Person other) {
    __isset_bit_vector.clear();
    __isset_bit_vector.or(other.__isset_bit_vector);
    this.id = TBaseHelper.deepCopy(other.id);
    if (other.isSetName()) {
      this.name = TBaseHelper.deepCopy(other.name);
    }
    this.age = TBaseHelper.deepCopy(other.age);
    if (other.isSetAddress()) {
      this.address = TBaseHelper.deepCopy(other.address);
    }
    if (other.isSetFavoriteColor()) {
      this.favoriteColor = TBaseHelper.deepCopy(other.favoriteColor);
    }
    if (other.isSetFriends()) {
      this.friends = TBaseHelper.deepCopy(other.friends);
    }
    this.bestFriend = TBaseHelper.deepCopy(other.bestFriend);
    if (other.isSetPetNames()) {
      this.petNames = TBaseHelper.deepCopy(other.petNames);
    }
    this.afraidOfAnimal = TBaseHelper.deepCopy(other.afraidOfAnimal);
    if (other.isSetVehicles()) {
      this.vehicles = TBaseHelper.deepCopy(other.vehicles);
    }
  }

  public Person deepCopy() {
    return new Person(this);
  }

  @Deprecated
  public Person clone() {
    return new Person(this);
  }

  public long getId() {
    return this.id;
  }

  public Person setId(long id) {
    this.id = id;
    setIdIsSet(true);
    return this;
  }

  public void unsetId() {
    __isset_bit_vector.clear(__ID_ISSET_ID);
  }

  // Returns true if field id is set (has been assigned a value) and false otherwise
  public boolean isSetId() {
    return __isset_bit_vector.get(__ID_ISSET_ID);
  }

  public void setIdIsSet(boolean __value) {
    __isset_bit_vector.set(__ID_ISSET_ID, __value);
  }

  public String getName() {
    return this.name;
  }

  public Person setName(String name) {
    this.name = name;
    return this;
  }

  public void unsetName() {
    this.name = null;
  }

  // Returns true if field name is set (has been assigned a value) and false otherwise
  public boolean isSetName() {
    return this.name != null;
  }

  public void setNameIsSet(boolean __value) {
    if (!__value) {
      this.name = null;
    }
  }

  public short getAge() {
    return this.age;
  }

  public Person setAge(short age) {
    this.age = age;
    setAgeIsSet(true);
    return this;
  }

  public void unsetAge() {
    __isset_bit_vector.clear(__AGE_ISSET_ID);
  }

  // Returns true if field age is set (has been assigned a value) and false otherwise
  public boolean isSetAge() {
    return __isset_bit_vector.get(__AGE_ISSET_ID);
  }

  public void setAgeIsSet(boolean __value) {
    __isset_bit_vector.set(__AGE_ISSET_ID, __value);
  }

  public String getAddress() {
    return this.address;
  }

  public Person setAddress(String address) {
    this.address = address;
    return this;
  }

  public void unsetAddress() {
    this.address = null;
  }

  // Returns true if field address is set (has been assigned a value) and false otherwise
  public boolean isSetAddress() {
    return this.address != null;
  }

  public void setAddressIsSet(boolean __value) {
    if (!__value) {
      this.address = null;
    }
  }

  public Color getFavoriteColor() {
    return this.favoriteColor;
  }

  public Person setFavoriteColor(Color favoriteColor) {
    this.favoriteColor = favoriteColor;
    return this;
  }

  public void unsetFavoriteColor() {
    this.favoriteColor = null;
  }

  // Returns true if field favoriteColor is set (has been assigned a value) and false otherwise
  public boolean isSetFavoriteColor() {
    return this.favoriteColor != null;
  }

  public void setFavoriteColorIsSet(boolean __value) {
    if (!__value) {
      this.favoriteColor = null;
    }
  }

  public Set<Long> getFriends() {
    return this.friends;
  }

  public Person setFriends(Set<Long> friends) {
    this.friends = friends;
    return this;
  }

  public void unsetFriends() {
    this.friends = null;
  }

  // Returns true if field friends is set (has been assigned a value) and false otherwise
  public boolean isSetFriends() {
    return this.friends != null;
  }

  public void setFriendsIsSet(boolean __value) {
    if (!__value) {
      this.friends = null;
    }
  }

  public long getBestFriend() {
    return this.bestFriend;
  }

  public Person setBestFriend(long bestFriend) {
    this.bestFriend = bestFriend;
    setBestFriendIsSet(true);
    return this;
  }

  public void unsetBestFriend() {
    __isset_bit_vector.clear(__BESTFRIEND_ISSET_ID);
  }

  // Returns true if field bestFriend is set (has been assigned a value) and false otherwise
  public boolean isSetBestFriend() {
    return __isset_bit_vector.get(__BESTFRIEND_ISSET_ID);
  }

  public void setBestFriendIsSet(boolean __value) {
    __isset_bit_vector.set(__BESTFRIEND_ISSET_ID, __value);
  }

  public Map<Animal,String> getPetNames() {
    return this.petNames;
  }

  public Person setPetNames(Map<Animal,String> petNames) {
    this.petNames = petNames;
    return this;
  }

  public void unsetPetNames() {
    this.petNames = null;
  }

  // Returns true if field petNames is set (has been assigned a value) and false otherwise
  public boolean isSetPetNames() {
    return this.petNames != null;
  }

  public void setPetNamesIsSet(boolean __value) {
    if (!__value) {
      this.petNames = null;
    }
  }

  /**
   * 
   * @see Animal
   */
  public Animal getAfraidOfAnimal() {
    return this.afraidOfAnimal;
  }

  /**
   * 
   * @see Animal
   */
  public Person setAfraidOfAnimal(Animal afraidOfAnimal) {
    this.afraidOfAnimal = afraidOfAnimal;
    setAfraidOfAnimalIsSet(true);
    return this;
  }

  public void unsetAfraidOfAnimal() {
    __isset_bit_vector.clear(__AFRAIDOFANIMAL_ISSET_ID);
  }

  // Returns true if field afraidOfAnimal is set (has been assigned a value) and false otherwise
  public boolean isSetAfraidOfAnimal() {
    return __isset_bit_vector.get(__AFRAIDOFANIMAL_ISSET_ID);
  }

  public void setAfraidOfAnimalIsSet(boolean __value) {
    __isset_bit_vector.set(__AFRAIDOFANIMAL_ISSET_ID, __value);
  }

  public List<Vehicle> getVehicles() {
    return this.vehicles;
  }

  public Person setVehicles(List<Vehicle> vehicles) {
    this.vehicles = vehicles;
    return this;
  }

  public void unsetVehicles() {
    this.vehicles = null;
  }

  // Returns true if field vehicles is set (has been assigned a value) and false otherwise
  public boolean isSetVehicles() {
    return this.vehicles != null;
  }

  public void setVehiclesIsSet(boolean __value) {
    if (!__value) {
      this.vehicles = null;
    }
  }

  @SuppressWarnings("unchecked")
  public void setFieldValue(int fieldID, Object __value) {
    switch (fieldID) {
    case ID:
      if (__value == null) {
        unsetId();
      } else {
        setId((Long)__value);
      }
      break;

    case NAME:
      if (__value == null) {
        unsetName();
      } else {
        setName((String)__value);
      }
      break;

    case AGE:
      if (__value == null) {
        unsetAge();
      } else {
        setAge((Short)__value);
      }
      break;

    case ADDRESS:
      if (__value == null) {
        unsetAddress();
      } else {
        setAddress((String)__value);
      }
      break;

    case FAVORITECOLOR:
      if (__value == null) {
        unsetFavoriteColor();
      } else {
        setFavoriteColor((Color)__value);
      }
      break;

    case FRIENDS:
      if (__value == null) {
        unsetFriends();
      } else {
        setFriends((Set<Long>)__value);
      }
      break;

    case BESTFRIEND:
      if (__value == null) {
        unsetBestFriend();
      } else {
        setBestFriend((Long)__value);
      }
      break;

    case PETNAMES:
      if (__value == null) {
        unsetPetNames();
      } else {
        setPetNames((Map<Animal,String>)__value);
      }
      break;

    case AFRAIDOFANIMAL:
      if (__value == null) {
        unsetAfraidOfAnimal();
      } else {
        setAfraidOfAnimal((Animal)__value);
      }
      break;

    case VEHICLES:
      if (__value == null) {
        unsetVehicles();
      } else {
        setVehicles((List<Vehicle>)__value);
      }
      break;

    default:
      throw new IllegalArgumentException("Field " + fieldID + " doesn't exist!");
    }
  }

  public Object getFieldValue(int fieldID) {
    switch (fieldID) {
    case ID:
      return new Long(getId());

    case NAME:
      return getName();

    case AGE:
      return new Short(getAge());

    case ADDRESS:
      return getAddress();

    case FAVORITECOLOR:
      return getFavoriteColor();

    case FRIENDS:
      return getFriends();

    case BESTFRIEND:
      return new Long(getBestFriend());

    case PETNAMES:
      return getPetNames();

    case AFRAIDOFANIMAL:
      return getAfraidOfAnimal();

    case VEHICLES:
      return getVehicles();

    default:
      throw new IllegalArgumentException("Field " + fieldID + " doesn't exist!");
    }
  }

  // Returns true if field corresponding to fieldID is set (has been assigned a value) and false otherwise
  public boolean isSet(int fieldID) {
    switch (fieldID) {
    case ID:
      return isSetId();
    case NAME:
      return isSetName();
    case AGE:
      return isSetAge();
    case ADDRESS:
      return isSetAddress();
    case FAVORITECOLOR:
      return isSetFavoriteColor();
    case FRIENDS:
      return isSetFriends();
    case BESTFRIEND:
      return isSetBestFriend();
    case PETNAMES:
      return isSetPetNames();
    case AFRAIDOFANIMAL:
      return isSetAfraidOfAnimal();
    case VEHICLES:
      return isSetVehicles();
    default:
      throw new IllegalArgumentException("Field " + fieldID + " doesn't exist!");
    }
  }

  @Override
  public boolean equals(Object that) {
    if (that == null)
      return false;
    if (that instanceof Person)
      return this.equals((Person)that);
    return false;
  }

  public boolean equals(Person that) {
    if (that == null)
      return false;
    if (this == that)
      return true;

    boolean this_present_id = true;
    boolean that_present_id = true;
    if (this_present_id || that_present_id) {
      if (!(this_present_id && that_present_id))
        return false;
      if (!TBaseHelper.equalsNobinary(this.id, that.id))
        return false;
    }

    boolean this_present_name = true && this.isSetName();
    boolean that_present_name = true && that.isSetName();
    if (this_present_name || that_present_name) {
      if (!(this_present_name && that_present_name))
        return false;
      if (!TBaseHelper.equalsNobinary(this.name, that.name))
        return false;
    }

    boolean this_present_age = true && this.isSetAge();
    boolean that_present_age = true && that.isSetAge();
    if (this_present_age || that_present_age) {
      if (!(this_present_age && that_present_age))
        return false;
      if (!TBaseHelper.equalsNobinary(this.age, that.age))
        return false;
    }

    boolean this_present_address = true && this.isSetAddress();
    boolean that_present_address = true && that.isSetAddress();
    if (this_present_address || that_present_address) {
      if (!(this_present_address && that_present_address))
        return false;
      if (!TBaseHelper.equalsNobinary(this.address, that.address))
        return false;
    }

    boolean this_present_favoriteColor = true && this.isSetFavoriteColor();
    boolean that_present_favoriteColor = true && that.isSetFavoriteColor();
    if (this_present_favoriteColor || that_present_favoriteColor) {
      if (!(this_present_favoriteColor && that_present_favoriteColor))
        return false;
      if (!TBaseHelper.equalsNobinary(this.favoriteColor, that.favoriteColor))
        return false;
    }

    boolean this_present_friends = true && this.isSetFriends();
    boolean that_present_friends = true && that.isSetFriends();
    if (this_present_friends || that_present_friends) {
      if (!(this_present_friends && that_present_friends))
        return false;
      if (!TBaseHelper.equalsNobinary(this.friends, that.friends))
        return false;
    }

    boolean this_present_bestFriend = true && this.isSetBestFriend();
    boolean that_present_bestFriend = true && that.isSetBestFriend();
    if (this_present_bestFriend || that_present_bestFriend) {
      if (!(this_present_bestFriend && that_present_bestFriend))
        return false;
      if (!TBaseHelper.equalsNobinary(this.bestFriend, that.bestFriend))
        return false;
    }

    boolean this_present_petNames = true && this.isSetPetNames();
    boolean that_present_petNames = true && that.isSetPetNames();
    if (this_present_petNames || that_present_petNames) {
      if (!(this_present_petNames && that_present_petNames))
        return false;
      if (!TBaseHelper.equalsNobinary(this.petNames, that.petNames))
        return false;
    }

    boolean this_present_afraidOfAnimal = true && this.isSetAfraidOfAnimal();
    boolean that_present_afraidOfAnimal = true && that.isSetAfraidOfAnimal();
    if (this_present_afraidOfAnimal || that_present_afraidOfAnimal) {
      if (!(this_present_afraidOfAnimal && that_present_afraidOfAnimal))
        return false;
      if (!TBaseHelper.equalsNobinary(this.afraidOfAnimal, that.afraidOfAnimal))
        return false;
    }

    boolean this_present_vehicles = true && this.isSetVehicles();
    boolean that_present_vehicles = true && that.isSetVehicles();
    if (this_present_vehicles || that_present_vehicles) {
      if (!(this_present_vehicles && that_present_vehicles))
        return false;
      if (!TBaseHelper.equalsNobinary(this.vehicles, that.vehicles))
        return false;
    }

    return true;
  }

  @Override
  public int hashCode() {
    return Arrays.deepHashCode(new Object[] {id, name, age, address, favoriteColor, friends, bestFriend, petNames, afraidOfAnimal, vehicles});
  }

  @Override
  public int compareTo(Person other) {
    if (other == null) {
      // See java.lang.Comparable docs
      throw new NullPointerException();
    }

    if (other == this) {
      return 0;
    }
    int lastComparison = 0;

    lastComparison = Boolean.valueOf(isSetId()).compareTo(other.isSetId());
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = TBaseHelper.compareTo(id, other.id);
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = Boolean.valueOf(isSetName()).compareTo(other.isSetName());
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = TBaseHelper.compareTo(name, other.name);
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = Boolean.valueOf(isSetAge()).compareTo(other.isSetAge());
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = TBaseHelper.compareTo(age, other.age);
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = Boolean.valueOf(isSetAddress()).compareTo(other.isSetAddress());
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = TBaseHelper.compareTo(address, other.address);
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = Boolean.valueOf(isSetFavoriteColor()).compareTo(other.isSetFavoriteColor());
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = TBaseHelper.compareTo(favoriteColor, other.favoriteColor);
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = Boolean.valueOf(isSetFriends()).compareTo(other.isSetFriends());
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = TBaseHelper.compareTo(friends, other.friends);
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = Boolean.valueOf(isSetBestFriend()).compareTo(other.isSetBestFriend());
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = TBaseHelper.compareTo(bestFriend, other.bestFriend);
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = Boolean.valueOf(isSetPetNames()).compareTo(other.isSetPetNames());
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = TBaseHelper.compareTo(petNames, other.petNames);
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = Boolean.valueOf(isSetAfraidOfAnimal()).compareTo(other.isSetAfraidOfAnimal());
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = TBaseHelper.compareTo(afraidOfAnimal, other.afraidOfAnimal);
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = Boolean.valueOf(isSetVehicles()).compareTo(other.isSetVehicles());
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = TBaseHelper.compareTo(vehicles, other.vehicles);
    if (lastComparison != 0) {
      return lastComparison;
    }
    return 0;
  }

  public void read(TProtocol iprot) throws TException {
    TField __field;
    iprot.readStructBegin(metaDataMap);
    while (true)
    {
      __field = iprot.readFieldBegin();
      if (__field.type == TType.STOP) { 
        break;
      }
      switch (__field.id)
      {
        case ID:
          if (__field.type == TType.I64) {
            this.id = iprot.readI64();
            setIdIsSet(true);
          } else { 
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case NAME:
          if (__field.type == TType.STRING) {
            this.name = iprot.readString();
          } else { 
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case AGE:
          if (__field.type == TType.I16) {
            this.age = iprot.readI16();
            setAgeIsSet(true);
          } else { 
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case ADDRESS:
          if (__field.type == TType.STRING) {
            this.address = iprot.readString();
          } else { 
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case FAVORITECOLOR:
          if (__field.type == TType.STRUCT) {
            this.favoriteColor = new Color();
            this.favoriteColor.read(iprot);
          } else { 
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case FRIENDS:
          if (__field.type == TType.SET) {
            {
              TSet _set0 = iprot.readSetBegin();
              this.friends = new HashSet<Long>(Math.max(0, 2*_set0.size));
              for (int _i1 = 0; 
                   (_set0.size < 0) ? iprot.peekSet() : (_i1 < _set0.size); 
                   ++_i1)
              {
                long _elem2;
                _elem2 = iprot.readI64();
                this.friends.add(_elem2);
              }
              iprot.readSetEnd();
            }
          } else { 
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case BESTFRIEND:
          if (__field.type == TType.I64) {
            this.bestFriend = iprot.readI64();
            setBestFriendIsSet(true);
          } else { 
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case PETNAMES:
          if (__field.type == TType.MAP) {
            {
              TMap _map3 = iprot.readMapBegin();
              this.petNames = new HashMap<Animal,String>(Math.max(0, 2*_map3.size));
              for (int _i4 = 0; 
                   (_map3.size < 0) ? iprot.peekMap() : (_i4 < _map3.size); 
                   ++_i4)
              {
                Animal _key5;
                String _val6;
                _key5 = Animal.findByValue(iprot.readI32());
                _val6 = iprot.readString();
                this.petNames.put(_key5, _val6);
              }
              iprot.readMapEnd();
            }
          } else { 
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case AFRAIDOFANIMAL:
          if (__field.type == TType.I32) {
            this.afraidOfAnimal = Animal.findByValue(iprot.readI32());
            setAfraidOfAnimalIsSet(true);
          } else { 
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case VEHICLES:
          if (__field.type == TType.LIST) {
            {
              TList _list7 = iprot.readListBegin();
              this.vehicles = new ArrayList<Vehicle>(Math.max(0, _list7.size));
              for (int _i8 = 0; 
                   (_list7.size < 0) ? iprot.peekList() : (_i8 < _list7.size); 
                   ++_i8)
              {
                Vehicle _elem9;
                _elem9 = new Vehicle();
                _elem9.read(iprot);
                this.vehicles.add(_elem9);
              }
              iprot.readListEnd();
            }
          } else { 
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        default:
          TProtocolUtil.skip(iprot, __field.type);
          break;
      }
      iprot.readFieldEnd();
    }
    iprot.readStructEnd();


    // check for required fields of primitive type, which can't be checked in the validate method
    validate();
  }

  public void write(TProtocol oprot) throws TException {
    validate();

    oprot.writeStructBegin(STRUCT_DESC);
    oprot.writeFieldBegin(ID_FIELD_DESC);
    oprot.writeI64(this.id);
    oprot.writeFieldEnd();
    if (this.name != null) {
      oprot.writeFieldBegin(NAME_FIELD_DESC);
      oprot.writeString(this.name);
      oprot.writeFieldEnd();
    }
    if (isSetAge()) {
      oprot.writeFieldBegin(AGE_FIELD_DESC);
      oprot.writeI16(this.age);
      oprot.writeFieldEnd();
    }
    if (this.address != null) {
      if (isSetAddress()) {
        oprot.writeFieldBegin(ADDRESS_FIELD_DESC);
        oprot.writeString(this.address);
        oprot.writeFieldEnd();
      }
    }
    if (this.favoriteColor != null) {
      if (isSetFavoriteColor()) {
        oprot.writeFieldBegin(FAVORITE_COLOR_FIELD_DESC);
        this.favoriteColor.write(oprot);
        oprot.writeFieldEnd();
      }
    }
    if (this.friends != null) {
      if (isSetFriends()) {
        oprot.writeFieldBegin(FRIENDS_FIELD_DESC);
        {
          oprot.writeSetBegin(new TSet(TType.I64, this.friends.size()));
          for (long _iter10 : this.friends)          {
            oprot.writeI64(_iter10);
          }
          oprot.writeSetEnd();
        }
        oprot.writeFieldEnd();
      }
    }
    if (isSetBestFriend()) {
      oprot.writeFieldBegin(BEST_FRIEND_FIELD_DESC);
      oprot.writeI64(this.bestFriend);
      oprot.writeFieldEnd();
    }
    if (this.petNames != null) {
      if (isSetPetNames()) {
        oprot.writeFieldBegin(PET_NAMES_FIELD_DESC);
        {
          oprot.writeMapBegin(new TMap(TType.I32, TType.STRING, this.petNames.size()));
          for (Map.Entry<Animal, String> _iter11 : this.petNames.entrySet())          {
            oprot.writeI32(_iter11.getKey() == null ? 0 : _iter11.getKey().getValue());
            oprot.writeString(_iter11.getValue());
          }
          oprot.writeMapEnd();
        }
        oprot.writeFieldEnd();
      }
    }
    if (isSetAfraidOfAnimal()) {
      oprot.writeFieldBegin(AFRAID_OF_ANIMAL_FIELD_DESC);
      oprot.writeI32(this.afraidOfAnimal == null ? 0 : this.afraidOfAnimal.getValue());
      oprot.writeFieldEnd();
    }
    if (this.vehicles != null) {
      if (isSetVehicles()) {
        oprot.writeFieldBegin(VEHICLES_FIELD_DESC);
        {
          oprot.writeListBegin(new TList(TType.STRUCT, this.vehicles.size()));
          for (Vehicle _iter12 : this.vehicles)          {
            _iter12.write(oprot);
          }
          oprot.writeListEnd();
        }
        oprot.writeFieldEnd();
      }
    }
    oprot.writeFieldStop();
    oprot.writeStructEnd();
  }

  @Override
  public String toString() {
    return toString(DEFAULT_PRETTY_PRINT);
  }

  @Override
  public String toString(boolean prettyPrint) {
    return toString(1, prettyPrint);
  }

  @Override
  public String toString(int indent, boolean prettyPrint) {
    String indentStr = prettyPrint ? TBaseHelper.getIndentedString(indent) : "";
    String newLine = prettyPrint ? "\n" : "";
    String space = prettyPrint ? " " : "";
    StringBuilder sb = new StringBuilder("Person");
    sb.append(space);
    sb.append("(");
    sb.append(newLine);
    boolean first = true;

    sb.append(indentStr);
    sb.append("id");
    sb.append(space);
    sb.append(":").append(space);
    sb.append(TBaseHelper.toString(this.getId(), indent + 1, prettyPrint));
    first = false;
    if (!first) sb.append("," + newLine);
    sb.append(indentStr);
    sb.append("name");
    sb.append(space);
    sb.append(":").append(space);
    if (this.getName() == null) {
      sb.append("null");
    } else {
      sb.append(TBaseHelper.toString(this.getName(), indent + 1, prettyPrint));
    }
    first = false;
    if (isSetAge())
    {
      if (!first) sb.append("," + newLine);
      sb.append(indentStr);
      sb.append("age");
      sb.append(space);
      sb.append(":").append(space);
      sb.append(TBaseHelper.toString(this.getAge(), indent + 1, prettyPrint));
      first = false;
    }
    if (isSetAddress())
    {
      if (!first) sb.append("," + newLine);
      sb.append(indentStr);
      sb.append("address");
      sb.append(space);
      sb.append(":").append(space);
      if (this.getAddress() == null) {
        sb.append("null");
      } else {
        sb.append(TBaseHelper.toString(this.getAddress(), indent + 1, prettyPrint));
      }
      first = false;
    }
    if (isSetFavoriteColor())
    {
      if (!first) sb.append("," + newLine);
      sb.append(indentStr);
      sb.append("favoriteColor");
      sb.append(space);
      sb.append(":").append(space);
      if (this.getFavoriteColor() == null) {
        sb.append("null");
      } else {
        sb.append(TBaseHelper.toString(this.getFavoriteColor(), indent + 1, prettyPrint));
      }
      first = false;
    }
    if (isSetFriends())
    {
      if (!first) sb.append("," + newLine);
      sb.append(indentStr);
      sb.append("friends");
      sb.append(space);
      sb.append(":").append(space);
      if (this.getFriends() == null) {
        sb.append("null");
      } else {
        sb.append(TBaseHelper.toString(this.getFriends(), indent + 1, prettyPrint));
      }
      first = false;
    }
    if (isSetBestFriend())
    {
      if (!first) sb.append("," + newLine);
      sb.append(indentStr);
      sb.append("bestFriend");
      sb.append(space);
      sb.append(":").append(space);
      sb.append(TBaseHelper.toString(this.getBestFriend(), indent + 1, prettyPrint));
      first = false;
    }
    if (isSetPetNames())
    {
      if (!first) sb.append("," + newLine);
      sb.append(indentStr);
      sb.append("petNames");
      sb.append(space);
      sb.append(":").append(space);
      if (this.getPetNames() == null) {
        sb.append("null");
      } else {
        sb.append(TBaseHelper.toString(this.getPetNames(), indent + 1, prettyPrint));
      }
      first = false;
    }
    if (isSetAfraidOfAnimal())
    {
      if (!first) sb.append("," + newLine);
      sb.append(indentStr);
      sb.append("afraidOfAnimal");
      sb.append(space);
      sb.append(":").append(space);
      String afraidOfAnimal_name = Animal.VALUES_TO_NAMES.get(this.getAfraidOfAnimal());
      if (afraidOfAnimal_name != null) {
        sb.append(afraidOfAnimal_name);
        sb.append(" (");
      }
      sb.append(this.getAfraidOfAnimal());
      if (afraidOfAnimal_name != null) {
        sb.append(")");
      }
      first = false;
    }
    if (isSetVehicles())
    {
      if (!first) sb.append("," + newLine);
      sb.append(indentStr);
      sb.append("vehicles");
      sb.append(space);
      sb.append(":").append(space);
      if (this.getVehicles() == null) {
        sb.append("null");
      } else {
        sb.append(TBaseHelper.toString(this.getVehicles(), indent + 1, prettyPrint));
      }
      first = false;
    }
    sb.append(newLine + TBaseHelper.reduceIndent(indentStr));
    sb.append(")");
    return sb.toString();
  }

  public void validate() throws TException {
    // check for required fields
  }

}

