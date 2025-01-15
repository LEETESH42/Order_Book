#include<iostream>
#include<cstdint>
#include<map>
#include<set>
#include<list>
#include<cmath>
#include<ctime>
#include<deque>
#include<stack>
#include<limits>
#include<string>
#include <vector>
#include<numeric>
#include<iostream>
#include<algorithm>
#include<unordered_map>
#include<memory>
#include<variant>
#include<optional>
#include<tuple>
#include<format>
#include<exception>
#include <cassert>

enum class OrderType
{
    GoodTillCancel,
    FillAndKill
};

enum class Side
{
    Buy,
    Sell
};

using Price=std::int32_t;
using Quantity=std::uint32_t;
using OrderId=std::uint64_t;

struct LevelInfo 
{
    Price price_;
    Quantity quantity_;
};

using LevelInfos=std::vector<LevelInfo>;

class OrderBookLevelInfos
{
public:
    OrderBookLevelInfos(const LevelInfos& bids, const LevelInfos& asks )
        :bids_{ bids }
        ,asks_{ asks }
    {}

    const LevelInfos& GetBids() const { return bids_; }
    const LevelInfos& GetAsks() const { return asks_ ;}

private:
    LevelInfos bids_;
    LevelInfos asks_;
};

class Order
{
public:
    Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
        :orderType_{ orderType }
        ,orderId_{ orderId }
        ,side_{ side }
        ,price_{ price }
        ,initialQuantity_{ quantity }
        ,remainingQuantity_{ quantity }
    { }

    OrderId GetOrderId() const { return orderId_; }
    Side GetSide() const { return side_; }
    Price GetPrice() const { return price_; }
    OrderType GetOrderType() const { return orderType_; }
    Quantity GetInitialQuantity() const { return initialQuantity_; }
    Quantity GetRemainingQuantity() const { return remainingQuantity_; }
    Quantity GetFilledQuantity() const { return GetInitialQuantity()- GetRemainingQuantity(); }
    bool IsFilled() const { return GetRemainingQuantity()==0;}

    void Fill(Quantity quantity){
        if(quantity > GetRemainingQuantity()){
            throw std::logic_error(std::format("Order ({}) cannot be filled for more than its remaining quantity.", GetOrderId()));
        }
        remainingQuantity_-=quantity;
    }

private:
    OrderType orderType_;
    OrderId orderId_;
    Side side_;
    Price price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;
};

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;

class OrderModify
{
public:
    OrderModify(OrderId orderId, Side side, Price price, Quantity quantity)
        :orderId_{orderId}
        ,price_{price}
        ,side_{side}
        ,quantity_{quantity}
    {}

    OrderId GetOrderId() const { return orderId_; }
    Price GetPrice() const { return price_; }
    Side GetSide()  const { return side_; } 
    Quantity GetQuantity() const { return quantity_; }

    OrderPointer ToOrderPointer(OrderType type) const
    {
        return std::make_shared<Order>(type, GetOrderId(), GetSide(), GetPrice(), GetQuantity());
    }

private:
    OrderId orderId_;
    Price price_;
    Side side_;
    Quantity quantity_; 
};

struct TradeInfo
{
    OrderId orderId_;
    Price price_;
    Quantity quantity_;
};

class Trade{
public:
    Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade)
        :bidTrade_{ bidTrade }
        ,askTrade_{ askTrade }
    {}

    const TradeInfo& GetBidTrade() const { return bidTrade_; }
    const TradeInfo& GetAskTrade() const { return askTrade_; }

private:
    TradeInfo bidTrade_;
    TradeInfo askTrade_;
};

using Trades = std::vector<Trade>;

class OrderBook
{
private:
    struct OrderEntry
    {
        OrderPointer order_{nullptr};
        OrderPointers::iterator location_;
    };

    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;

    bool CanMatch(Side side, Price price)const
    {
        if(side==Side::Buy){
            if(asks_.empty()){
                return false;
            }
            const auto& [bestAsk,_]=*asks_.begin();
            return price>=bestAsk;
        }
        else{
            
            if(bids_.empty()){
                return false;
            }
            const auto& [bestBid,_]=*bids_.begin();
            return price<=bestBid;
        }
    }

    Trades MatchOrders()
    {
        Trades trades;
        trades.reserve(orders_.size());

        while(true){
            if(bids_.empty() || asks_.empty()){
                break;
            }
            auto& [bidPrice,bids]=*bids_.begin();
            auto& [askPrice,asks]=*asks_.begin();

            if(bidPrice<askPrice){
                break;
            }

            while(bids.size() && asks.size()){
                auto& bid=bids.front();
                auto& ask=asks.front();

                Quantity quantity=std::min(bid->GetRemainingQuantity(),ask->GetRemainingQuantity());

                bid->Fill(quantity);
                ask->Fill(quantity);

                if(bid->IsFilled()){
                    bids.pop_front();
                    orders_.erase(bid->GetOrderId());
                }
                if(ask->IsFilled()){
                    asks.pop_front();
                    orders_.erase(ask->GetOrderId());
                }

                if(bids.empty()){
                    bids_.erase(bidPrice);
                }
                if(asks.empty()){
                    asks_.erase(askPrice);
                }

                trades.push_back(Trade{TradeInfo{bid->GetOrderId(), bid->GetPrice(), quantity},
                TradeInfo{ask->GetOrderId(),ask->GetPrice(), quantity}
                });

            }

        }

        if(!bids_.empty())
        {
            auto& [_,bids]=*bids_.begin();
            auto& order=bids.front();
            if(order->GetOrderType()==OrderType::FillAndKill){
                CancelOrder(order->GetOrderId());
            }
        }

        if(!asks_.empty()){
            auto& [_,asks]=*asks_.begin();
            auto& order=asks.front();
            if(order->GetOrderType()==OrderType::FillAndKill){
                CancelOrder(order->GetOrderId());
            }
        }
        return trades;
    }

public:
    Trades AddOrder(OrderPointer order)
    {
        if(orders_.contains(order->GetOrderId())){
            return {};
        }
        if(order->GetOrderType()==OrderType::FillAndKill && !CanMatch(order->GetSide(), order->GetPrice())){
            return {};
        }

        OrderPointers::iterator iterator;
        
        if(order->GetSide()==Side::Buy)
        {
            auto & orders=bids_[order->GetPrice()];
            orders.push_back(order);
            iterator=std::next(orders.begin(),orders.size()-1);
        }

        else{
            auto & orders=asks_[order->GetPrice()];
            orders.push_back(order);
            iterator=std::next(orders.begin(),orders.size()-1);
        }
        orders_.insert({order->GetOrderId(), OrderEntry{order,iterator}});
        return MatchOrders();
    }

    void CancelOrder(OrderId orderId)
    {
        if(!orders_.contains(orderId))
        {
            return ;
        }
        const auto& [order,iterator]=orders_.at(orderId);
        orders_.erase(orderId);

        if(order->GetSide()==Side::Sell)
        {
            auto price=order->GetPrice();
            auto & orders=asks_.at(price);
            orders.erase(iterator);
            if(orders.empty()){
                asks_.erase(price);
            }
        }
        else{
            auto price=order->GetPrice();
            auto& orders=bids_.at(price);
            orders.erase(iterator);
            if(orders.empty())
            {
                bids_.erase(price);
            }
        }
    }

    Trades MatchOrder(OrderModify order)
    {
        if(!orders_.contains(order.GetOrderId()))
        {
            return {};
        }
        const auto& [existingOrder, _]=orders_.at(order.GetOrderId());
        CancelOrder(order.GetOrderId());
        return AddOrder(order.ToOrderPointer(existingOrder->GetOrderType()));
    }

    std::size_t Size() const { return orders_.size(); }

    OrderBookLevelInfos GetOrderInfos() const 
    {
        LevelInfos bidInfos,askInfos;
        bidInfos.reserve(orders_.size());
        askInfos.reserve(orders_.size());

        auto CreateLevelInfos = [](Price price, const OrderPointers& orders)
        {
            return LevelInfo{ price, std::accumulate(orders.begin(),orders.end(),(Quantity)0,
                [](Quantity runningSum, const OrderPointer& order)
                { return runningSum+order->GetRemainingQuantity(); })};
        };

        for(const auto& [price, orders]:bids_)
        {
            bidInfos.push_back(CreateLevelInfos(price,orders));
        }

        for(const auto& [price, orders]: asks_){
            askInfos.push_back(CreateLevelInfos(price,orders));
        }
        return OrderBookLevelInfos{bidInfos,askInfos};

    }

};
 
void runOrderBookTests() {
    OrderBook orderbook;
    
    // Test Case 1: Basic Order Addition and Cancellation
    std::cout << "\nTest Case 1: Basic Order Addition and Cancellation" << std::endl;
    {
        // Add a buy order
        auto order = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Buy, 100, 10);
        auto trades = orderbook.AddOrder(order);
        
        assert(orderbook.Size() == 1);
        assert(trades.empty());
        
        // Cancel the order
        orderbook.CancelOrder(1);
        assert(orderbook.Size() == 0);
        
        std::cout << "Test Case 1 Passed!" << std::endl;
    }
    
    // Test Case 2: Order Matching
    std::cout << "\nTest Case 2: Order Matching" << std::endl;
    {
        // Add a buy order
        auto buyOrder = std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Buy, 100, 5);
        orderbook.AddOrder(buyOrder);
        
        // Add a matching sell order
        auto sellOrder = std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Sell, 100, 5);
        auto trades = orderbook.AddOrder(sellOrder);
        
        assert(trades.size() == 1);
        assert(trades[0].GetBidTrade().orderId_ == 2);
        assert(trades[0].GetAskTrade().orderId_ == 3);
        assert(trades[0].GetBidTrade().quantity_ == 5);
        assert(orderbook.Size() == 0);  // Both orders should be fully matched
        
        std::cout << "Test Case 2 Passed!" << std::endl;
    }
    
    // Test Case 3: Partial Fill
    std::cout << "\nTest Case 3: Partial Fill" << std::endl;
    {
        // Add a buy order for 10 units
        auto buyOrder = std::make_shared<Order>(OrderType::GoodTillCancel, 4, Side::Buy, 100, 10);
        orderbook.AddOrder(buyOrder);
        
        // Add a sell order for 6 units
        auto sellOrder = std::make_shared<Order>(OrderType::GoodTillCancel, 5, Side::Sell, 100, 6);
        auto trades = orderbook.AddOrder(sellOrder);
        
        assert(trades.size() == 1);
        assert(trades[0].GetBidTrade().quantity_ == 6);
        assert(orderbook.Size() == 1);  // Buy order should still be there with remaining quantity
        
        // Check order book levels
        auto levels = orderbook.GetOrderInfos();
        assert(levels.GetBids()[0].quantity_ == 4);  // Remaining quantity
        
        std::cout << "Test Case 3 Passed!" << std::endl;
    }

    // Test Case 4: FillAndKill Order
    std::cout << "\nTest Case 4: FillAndKill Order" << std::endl;
    {
        OrderBook newOrderbook;  // Start with fresh orderbook
        
        // Add a standing sell order
        auto sellOrder = std::make_shared<Order>(OrderType::GoodTillCancel, 6, Side::Sell, 100, 5);
        newOrderbook.AddOrder(sellOrder);
        
        // Add a FillAndKill buy order that can be partially filled
        auto buyOrder = std::make_shared<Order>(OrderType::FillAndKill, 7, Side::Buy, 100, 10);
        auto trades = newOrderbook.AddOrder(buyOrder);
        
        assert(trades.size() == 1);
        assert(trades[0].GetBidTrade().quantity_ == 5);
        assert(newOrderbook.Size() == 0);  // FillAndKill order should be cancelled after partial fill
        
        std::cout << "Test Case 4 Passed!" << std::endl;
    }
}

int main(){

    // OrderBook orderbook;
    // const OrderId orderId=1;
    // orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel, orderId, Side::Buy,100,10));
    // std::cout<<orderbook.Size()<<std::endl;
    // orderbook.CancelOrder(orderId);
    // std::cout<<orderbook.Size()<<std::endl;

        runOrderBookTests();


    return 0;
}